#include "decoder.h"
// #include "reader.h"
// #include "jpegreader.h"
#include <iostream>
#include <istream>
#include <sstream>
#include <stdexcept>
#include <glog/logging.h>
#include <map>
#include <string>
#include <unordered_map>
#include "huffman.h"
#include "readers.h"
#include "fft.h"
#include "decode_utils.h"
#include "math.h"

Image Decode(std::istream& input) {
    JpegReader bytessep(&input);
    //  DLOG(INFO) << "mcu bytes = " << bytessep.bytes_mcu_.size();
    //  DLOG(INFO) << "good read";
    for (const auto& it : bytessep.cnts) {
        if (it.second == 0) {
            throw std::runtime_error("jpg don't have some marker");
        }
        //  DLOG(INFO) << "cnt : " << it.first << " = " << it.second;
    }
    if (bytessep.cnts["EOI"] == 0) {
        throw std::runtime_error("cant find eoi");
    }
    if (bytessep.cnts["SOS"] != bytessep.cnts["SOSF0"]) {
        throw std::runtime_error(std::to_string(bytessep.cnts["SOS"]) + " " +
                                 std::to_string(bytessep.cnts["SOSf0"]));
    }

    if (bytessep.cnts["SOS"] != bytessep.cnts["EOI"]) {
        throw std::runtime_error(std::to_string(bytessep.cnts["SOS"]) + " " +
                                 std::to_string(bytessep.cnts["SOSf0"]));
    }

    Image result;
    // return result;

    std::string comment(bytessep.comment_.begin(), bytessep.comment_.end());
    result.SetComment(comment);
    // return result;

    UnZigZug get_curr_matrixes;
    std::unordered_map<uint8_t, HuffmanTree> dc_hm;
    std::unordered_map<uint8_t, HuffmanTree> ac_hm;

    for (const auto& it : bytessep.dht_dc_) {
        auto t = it.second;
        std::vector<uint8_t> coefs(t.begin(), t.begin() + 16);
        std::vector<uint8_t> values(t.begin() + 16, t.end());
        dc_hm[it.first].Build(coefs, values);
    }

    //  DLOG(INFO) << "build DC trees";
    for (const auto& it : bytessep.dht_ac_) {
        auto t = it.second;
        std::vector<uint8_t> coefs(t.begin(), t.begin() + 16);
        std::vector<uint8_t> values(t.begin() + 16, t.end());
        ac_hm[it.first].Build(coefs, values);
    }
    //  DLOG(INFO) << "build AC trees";
    //  DLOG(INFO) << comment;
    if (bytessep.meta_mcu_.empty()) {
        throw std::runtime_error("meta mcu empty");
    }
    uint8_t count_canals = bytessep.meta_mcu_[0];

    int dc_index[3];
    int ac_index[3];
    int matrix_quant[3];
    int horizontal_pr[3];
    int vertical_pr[3];
    if (bytessep.meta_[5] != bytessep.meta_mcu_.front()) {
        throw std::runtime_error("wrong sizes canals");
    }
    for (size_t i = 1; i <= count_canals * 2; i += 2) {
        //  DLOG(INFO) << bytessep.meta_mcu_[i] - 1 << "  --- index dc_ac "
        // << static_cast<int>(bytessep.meta_mcu_[i + 1]);
        if (i >= bytessep.meta_mcu_.size()) {
            throw std::runtime_error("hype");
        }
        if (bytessep.meta_mcu_[i] > count_canals) {
            throw std::runtime_error("out of bound");
        }
        dc_index[bytessep.meta_mcu_[i] - 1] = bytessep.meta_mcu_[i + 1] >> 4;
        ac_index[bytessep.meta_mcu_[i] - 1] = bytessep.meta_mcu_[i + 1] & 15;
        if (!bytessep.dht_dc_.count(dc_index[bytessep.meta_mcu_[i] - 1])) {
            throw std::runtime_error("no dc");
        }
        if (!bytessep.dht_ac_.count(ac_index[bytessep.meta_mcu_[i] - 1])) {
            throw std::runtime_error("no ac");
        }
    }
    for (size_t i = 6; i < bytessep.meta_.size(); i += 3) {
        //  DLOG(INFO) << "Meta info : " << static_cast<int>(bytessep.meta_[i]) << " : "
        // << static_cast<int>(bytessep.meta_[i + 1]) << " "
        // << static_cast<int>(bytessep.meta_[i + 2]);
        matrix_quant[bytessep.meta_[i] - 1] = bytessep.meta_[i + 2];
        if (!bytessep.dqts_.count(bytessep.meta_[i + 2])) {
            throw std::runtime_error("hasn't dqt");
        }
        if (bytessep.meta_[i] > count_canals) {
            throw std::runtime_error("out of bound");
        }
        horizontal_pr[bytessep.meta_[i] - 1] = bytessep.meta_[i + 1] >> 4;
        vertical_pr[bytessep.meta_[i] - 1] = bytessep.meta_[i + 1] & 15;
    }

    std::vector<std::vector<int16_t>> matrixes[3];
    std::string mcub(bytessep.bytes_mcu_.begin(), bytessep.bytes_mcu_.end());
    std::stringstream mcu(mcub);

    Readed<uint16_t, std::stringstream> mcureader(&mcu);

    int cnt = 0;
    int e = 0;
    int k = 0;
    if (bytessep.meta_.size() < 5) {
        throw std::runtime_error("meta too small");
    }
    uint16_t height = bytessep.meta_[1];
    height = (height << 8) + bytessep.meta_[2];
    uint16_t width = bytessep.meta_[3];
    width = (width << 8) + bytessep.meta_[4];
    if (width == 0 || height == 0) {
        throw std::runtime_error("invalid size");
    }
    result.SetSize(width, height);
    if (width % (8 * horizontal_pr[0]) != 0) {
        width = width + (8 * horizontal_pr[0]) - width % (8 * horizontal_pr[0]);
    }
    if (height % (8 * vertical_pr[0]) != 0) {
        height = height + (8 * vertical_pr[0]) - height % (8 * vertical_pr[0]);
    }

    //  DLOG(INFO) << "W, H : " << width << " " << height;
    int count_for_read = (height / 8) * (width / 8) + (count_canals - 1) * (height / 8) *
                                                          (width / 8) /
                                                          (horizontal_pr[0] * vertical_pr[0]);
    //  DLOG(INFO) << "count for read : " << count_for_read;
    int sum_mcu = 0;
    std::vector<int> sz_mcu;
    for (size_t i = 0; i < count_canals; ++i) {
        sum_mcu += horizontal_pr[i] * vertical_pr[i];
        sz_mcu.push_back(horizontal_pr[i] * vertical_pr[i]);
        //  DLOG(INFO) << "hr and vr : " << horizontal_pr[i] << " " << vertical_pr[i];
    }
    // return result;
    matrixes[0].reserve(count_for_read);
    matrixes[1].reserve(count_for_read);
    matrixes[2].reserve(count_for_read);
    while (!mcureader.IsEof()) {
        //  DLOG(INFO) << "SIZE : " << (height / 8) * (height / 8) * 3 << " " << e;
        if (e == count_for_read) {
            break;
        }
        e++;
        //  DLOG(INFO) << "---------------------------------------------------------";
        k %= sum_mcu;
        cnt = k;
        int w = 0;
        for (size_t i = 0; i < count_canals; ++i) {
            w += sz_mcu[i];
            if (w > k) {
                cnt = i;
                break;
            }
        }
        HuffmanTree& hdc = dc_hm[dc_index[cnt]];
        HuffmanTree& hac = ac_hm[ac_index[cnt]];
        int value;
        //  DLOG(INFO) << " CNT : " << cnt;
        while (42) {
            if (mcureader.IsEof()) {
                throw std::runtime_error("eof :(");
            }
            bool e = mcureader.ReadBits(1);
            // //  DLOG(INFO) << e;
            if (hdc.Move(e, value)) {
                break;
            }
        }
        //  DLOG(INFO) << "DC OK " << value;
        std::vector<int16_t> block;
        block.reserve(64);
        int16_t result_value = InvertWord(mcureader.ReadBits(value), value);
        block.push_back(result_value);
        while (block.size() < 64) {
            if (mcureader.IsEof()) {
                throw std::runtime_error("eof :(");
            }
            while (42) {
                if (mcureader.IsEof()) {
                    throw std::runtime_error("eof :(");
                }
                bool e = mcureader.ReadBits(1);
                if (hac.Move(e, value)) {
                    break;
                }
            }
            if (value == 0) {
                while (block.size() < 64) {
                    block.push_back(0);
                }
                break;
            }
            uint8_t g = (value >> 4);
            for (size_t j = 0; j < g; ++j) {
                block.push_back(0);
            }
            int16_t result_value = InvertWord(mcureader.ReadBits(value & 15), value & 15);
            block.push_back(result_value);
        }
        if (block.size() != 64) {
            //  DLOG(INFO) << "block size : " << block.size();
            throw std::invalid_argument("aaaaaaaaaa");
        }
        matrixes[cnt].push_back(block);
        k++;
    }
    //  DLOG(INFO) << "PART AFTER CORRECT DECODING SOS";
    std::vector<int16_t> quants[3];
    for (const auto& it : bytessep.dqts_) {
        std::vector<int16_t> h;
        for (const auto& value : it.second) {
            h.push_back(static_cast<int16_t>(value));
        }
        //  DLOG(INFO) << "QUAT";
        get_curr_matrixes.PrintMatrix(h);
        quants[it.first] = h;
    }
    // return result;
    for (size_t i = 0; i < count_canals; ++i) {
        for (size_t j = 1; j < matrixes[i].size(); ++j) {
            matrixes[i][j][0] += matrixes[i][j - 1][0];
        }
    }
    for (size_t i = 0; i < count_canals; ++i) {
        //  DLOG(INFO) << "Y";
        for (auto& part : matrixes[i]) {
            for (size_t j = 0; j < 64; ++j) {
                part[j] *= quants[matrix_quant[i]][j];
            }
            // //  DLOG(INFO) << "after quant : ";
            get_curr_matrixes.PrintMatrix(part);
            std::vector<double> out_elems(64), in_elems(64);
            std::vector<int16_t> g = get_curr_matrixes.GetVector(part);  // ???
            std::string kek;
            for (size_t j = 0; j < 64; ++j) {
                if (j && j % 8 == 0) {
                    // //  DLOG(INFO) << kek;
                    kek = "";
                }
                kek += std::to_string(g[j]);
                kek += " ";

                in_elems[j] = g[j];
            }
            DctCalculator dst(8, &in_elems, &out_elems);
            dst.Inverse();
            // //  DLOG(INFO) << "matrix after fftw";
            for (size_t j = 0; j < 64; ++j) {
                part[j] = round(out_elems[j]);
            }
            // //  DLOG(INFO) << kek;
        }
    }
    for (size_t i = 0; i < 3; ++i) {
        //  DLOG(INFO) << i << " : " << horizontal_pr[i] << " " << vertical_pr[i];
    }
    std::vector<std::vector<int16_t>> canal_y(height, std::vector<int16_t>(width, 128));
    std::vector<std::vector<int16_t>> canal_cb(height, std::vector<int16_t>(width, 128));
    std::vector<std::vector<int16_t>> canal_cr(height, std::vector<int16_t>(width, 128));
    int height_exp = horizontal_pr[0];
    int width_exp = vertical_pr[0];
    std::swap(width_exp, height_exp);
    // return result;

    //  DLOG(INFO) << "y canal"
    //  << " " << height << " " << width << " " << height_exp << " " << width_exp;
    for (size_t i = 0; i < matrixes[0].size(); ++i) {
        int val = i / (width_exp * height_exp);
        int dx = val / (width / (8 * width_exp));
        int dy = val % (width / (8 * width_exp));
        dx *= 8 * height_exp;
        dy *= 8 * width_exp;
        val = i % (width_exp * height_exp);
        dx += val / width_exp * 8;
        dy += val % width_exp * 8;
        //  DLOG(INFO) << "Y KEK " << dx << " " << dy;
        for (size_t x = 0; x < 8; ++x) {
            for (size_t y = 0; y < 8; ++y) {
                // //  DLOG(INFO) << "Y VAR : " << dx + x << " " << dy + y << " : " << 8 * x + y;
                canal_y[dx + x][dy + y] =
                    std::min(255, std::max(0, matrixes[0][i][x * 8 + y] + 128));
            }
        }
    }

    for (size_t i = 0; i < matrixes[1].size(); ++i) {
        int val = i;
        int dx = val / (width / (8 * width_exp));
        int dy = val % (width / (8 * width_exp));
        dx *= 8 * height_exp;
        dy *= 8 * width_exp;
        //  DLOG(INFO) << "Cb KEK " << dx << " " << dy;
        for (int x = 0; x < 8 * height_exp; ++x) {
            for (int y = 0; y < 8 * width_exp; ++y) {
                // //  DLOG(INFO) << "cb VAR : " << dx + x << " " << dy + y << " : " << x /
                // height_exp * 8 + y / width_exp;
                canal_cb[dx + x][dy + y] = std::min<short>(
                    255,
                    std::max<short>(0, matrixes[1][i][x / height_exp * 8 + y / width_exp] + 128));
            }
        }
    }

    // //  DLOG(INFO) << "cr canal" << matrixes[2].size();
    for (size_t i = 0; i < matrixes[2].size(); ++i) {
        int val = i;
        int dx = val / (width / (8 * width_exp));
        int dy = val % (width / (8 * width_exp));
        dx *= 8 * height_exp;
        dy *= 8 * width_exp;
        //  DLOG(INFO) << "Cr KEK " << dx << " " << dy;
        for (int x = 0; x < 8 * height_exp; ++x) {
            for (int y = 0; y < 8 * width_exp; ++y) {
                // //  DLOG(INFO) << x / height_exp * 8 + y / width_exp;
                // //  DLOG(INFO) << "cr VAR : " << dx + x << " " << dy + y << " : " << x /
                // height_exp * 8 + y / width_exp;
                canal_cr[dx + x][dy + y] = std::min<short>(
                    255,
                    std::max<short>(0, matrixes[2][i][x / height_exp * 8 + y / width_exp] + 128));
            }
        }
    }
    // return result;
    height = bytessep.meta_[1];
    height = (height << 8) + bytessep.meta_[2];
    width = bytessep.meta_[3];
    width = (width << 8) + bytessep.meta_[4];
    for (size_t i = 0; i < height; ++i) {
        std::string kek;
        kek = "";
        for (size_t j = 0; j < width; ++j) {
            RGB f;
            f.r = round(1. * canal_y[i][j] + 1.402 * (canal_cr[i][j] - 128));
            f.g = round(1. * canal_y[i][j] - 0.34414 * (canal_cb[i][j] - 128) -
                        0.71414 * (canal_cr[i][j] - 128));
            f.b = round(1. * canal_y[i][j] + 1.772 * (canal_cb[i][j] - 128));
            // //  DLOG(INFO) << "CARGBS : " << i << " " << j << " : " << f.r << " " << f.g << " "
            // << f.b;
            f.r = std::min(255, std::max(f.r, 0));
            f.g = std::min(255, std::max(f.g, 0));
            f.b = std::min(255, std::max(f.b, 0));
            result.SetPixel(i, j, f);
            if (i < 8 && j < 8) {
                kek += std::to_string(f.b) + " ";
            }
        }
        //  DLOG(INFO) << kek;
    }

    //  DLOG(INFO) << "OK";

    return result;
}

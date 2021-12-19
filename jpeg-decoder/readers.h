#pragma once
#include <stdlib.h>
#include <map>
#include <stdexcept>
#include <unordered_map>
#include <glog/logging.h>

const uint8_t kMarkerFF = 0xFF;
const uint8_t kMarkerSOI = 0xD8;
const uint8_t kMarkerEOI = 0xD9;
const uint8_t kMarkerCom = 0xFE;
const uint8_t kMarkerAppns = 0xE0;
const uint8_t kMarkerAppnf = 0xEF;
const uint8_t kMarkerDQT = 0xDB;
const uint8_t kMarkerDHT = 0xC4;
const uint8_t kMarkerSOSF0 = 0xC0;
const uint8_t kMarkerSOS = 0xDA;

template <typename T, typename F>
class Readed {
public:
    Readed(F* input) : input_(input) {
    }

    void ReadForAllBuff() {

        ptr_buf_ = 0;
        true_size_ = 0;
        for (size_t i = 0; i < kBufferSize / wordsize_; ++i) {
            if (input_->eof()) {
                break;
            }
            true_size_ = (i + 1) * wordsize_;
            int g = input_->get();
            // DLOG(INFO) << "READER read i : " << i << " value : " << g;
            T bt = g;
            for (size_t j = 0; j < wordsize_; ++j) {
                buffer_[i * wordsize_ + j] = bt & (1 << (wordsize_ - 1 - j));
            }
        }
        if (!true_size_) {
            is_eof_ = true;
        }
    }

    T ReadBits(size_t cnt = 8) {
        T result = T();
        for (size_t i = 0; i < cnt; ++i) {
            if (ptr_buf_ == true_size_) {
                ReadForAllBuff();
                if (is_eof_) {
                    // throw std::runtime_error("try read eof");
                    return T();
                }
            }
            result <<= 1;
            if (buffer_[ptr_buf_++]) {
                result += 1;
            }
        }
        count_read_ += cnt;
        return result;
    }

    T Peek(size_t cnt = 8) {
        T result = ReadBits(cnt);
        if (is_eof_) {
            return result;
        }
        ptr_buf_ -= cnt;
        count_read_ -= cnt;
        return result;
    }

    bool IsEof() const {
        return is_eof_;
    }

    int count_read_ = 0;

private:
    bool is_eof_ = false;
    F* input_;
    const static size_t kBufferSize = 4096;
    const size_t wordsize_ = 8;
    bool buffer_[kBufferSize];
    size_t ptr_buf_ = 0;
    size_t true_size_ = 0;
};

class JpegReader {
private:
public:
    JpegReader(std::istream* input) : reader_(input) {
        if (reader_.ReadBits() != kMarkerFF) {
            throw std::runtime_error("not ff in start");
        }
        if (reader_.Peek() != kMarkerSOI) {
            throw std::runtime_error("not soi in start");
        }
        DLOG(INFO) << "jpegreader initializtion";
        if (!Controller()) {
            throw std::runtime_error("some fuckup");
        }
    }

    bool Controller() {  // return true if we step to the next part, called after ff, using peek
        DLOG(INFO) << "read bytes : " << reader_.count_read_ / 8;
        if (reader_.IsEof()) {
            // throw std::runtime_error("eof withot eoi");
            DLOG(ERROR) << "eof without eoi";
            return true;
        }
        if (reader_.Peek() == kMarkerSOI) {
            reader_.ReadBits();
            DLOG(INFO) << "SOI";
            cnts["SOI"]++;
            SoiStart();
            return true;
        }
        if (reader_.Peek() == 0) {  // ????
            reader_.ReadBits();
            return false;
        }
        if (reader_.Peek() == kMarkerEOI) {
            reader_.ReadBits();
            DLOG(INFO) << "EOI";
            cnts["EOI"]++;
            if (reader_.Peek() == kMarkerFF) {
                reader_.ReadBits();
                Controller();
            }
            return true;
        }
        if (reader_.Peek() == kMarkerCom) {
            reader_.ReadBits();
            DLOG(INFO) << "COM";
            cnts["COM"]++;
            ComStart();
            return true;
        }
        if (reader_.Peek() >= kMarkerAppns && reader_.Peek() <= kMarkerAppnf) {
            reader_.ReadBits();
            DLOG(INFO) << "Appn";
            cnts["Appn"]++;
            AppnStart();
            return true;
        }

        if (reader_.Peek() == kMarkerDQT) {
            DLOG(INFO) << "DQT";
            cnts["DQT"]++;
            reader_.ReadBits();
            DQTStart();
            return true;
        }

        if (reader_.Peek() == kMarkerDHT) {
            reader_.ReadBits();
            // cnts["DHT"]++;
            DLOG(INFO) << "DHT";
            DHTStart();
            return true;
        }
        if (reader_.Peek() == kMarkerSOSF0) {
            DLOG(INFO) << "SOSf0";
            cnts["SOSF0"]++;
            reader_.ReadBits();
            SOSF0Start();
            return true;
        }

        if (reader_.Peek() == kMarkerSOS) {
            DLOG(INFO) << "SOS";
            cnts["SOS"]++;
            reader_.ReadBits();
            SOSStart();
            return true;
        }

        throw std::runtime_error("controler flex");
    }

    // recheck повторения маркеров!
    // написать функцию считывания байтов, пока копипаста
    // я не сразу осознал что размеры написаны везде, думал от маркера до маркера идём

    void AppnStart() {  // change to read
        uint16_t sz = reader_.ReadBits();
        uint16_t b = reader_.ReadBits();
        sz = (sz << 8) + b;
        DLOG(INFO) << "appn size : " << sz;
        std::vector<uint8_t> matrix;
        for (uint16_t i = 0; i + 2 < sz; ++i) {
            if (reader_.IsEof()) {
                DLOG(ERROR) << " appn " << i + 1;
                throw std::runtime_error("appn end");
            }
            reader_.ReadBits();
        }
        uint8_t r = reader_.ReadBits();
        if (r != kMarkerFF) {
            throw std::runtime_error("after com not ff");
        }
        if (!Controller()) {
            DLOG(ERROR) << "prethrow com no marker : " << static_cast<int>(reader_.Peek());
            throw std::runtime_error("after com no marker :(");
        }
    }

    void ComStart() {  // change to read
        uint16_t sz = reader_.ReadBits();
        uint16_t b = reader_.ReadBits();
        sz = (sz << 8) + b;
        DLOG(INFO) << "com size : " << sz;
        std::vector<uint8_t> matrix;
        for (uint16_t i = 0; i + 2 < sz; ++i) {
            if (reader_.IsEof()) {
                DLOG(ERROR) << " com " << i + 1;
                throw std::runtime_error("com end");
            }
            uint16_t value = reader_.ReadBits();
            matrix.push_back(value);
        }
        DLOG(INFO) << "com MATRIX SIZE: " << matrix.size();
        comment_ = matrix;
        uint8_t r = reader_.ReadBits();
        if (r != kMarkerFF) {
            throw std::runtime_error("after com not ff");
        }
        if (!Controller()) {
            DLOG(ERROR) << "prethrow com no marker : " << static_cast<int>(reader_.Peek());
            throw std::runtime_error("after com no marker :(");
        }
    }
    void SoiStart() {  // change to read
        while (!reader_.IsEof()) {
            uint8_t t = reader_.ReadBits();
            DLOG(INFO) << "SOI FLEX";
            if (t == kMarkerFF) {
                if (Controller()) {
                    return;
                }
            }
            throw std::runtime_error("some flex with bytes");
        }
        Controller();
    }

    void DQTStart() {
        uint16_t sz = reader_.ReadBits();
        uint16_t b = reader_.ReadBits();
        sz = (sz << 8) + b;
        DLOG(INFO) << "DQT size : " << sz;
        sz -= 2;
        uint16_t i = 0;
        while (i < sz) {
            uint8_t ident = reader_.ReadBits();
            uint8_t number = ident & 15;
            uint8_t szbytes = (ident >> 4) & 15;
            szbytes += 1;
            i++;
            DLOG(INFO) << "DQT num + szbytes : " << static_cast<int>(number) << " "
                       << static_cast<int>(szbytes);
            std::vector<uint16_t> matrix;
            for (uint16_t j = 0; j < 64 * szbytes; j += szbytes) {
                if (reader_.IsEof()) {
                    DLOG(ERROR) << " DQT EOF " << i + 1;
                    throw std::runtime_error("DQT end");
                }
                uint16_t value = reader_.ReadBits();
                if (szbytes == 2) {
                    value <<= 8;
                    i++;
                    value += reader_.ReadBits();
                }
                matrix.push_back(value);
                ++i;
            }
            DLOG(INFO) << "DQT MATRIX SIZE: " << matrix.size();
            dqts_[ident] = matrix;
        }
        if (i > sz) {
            throw std::runtime_error("dqt problems");
        }
        uint8_t r = reader_.ReadBits();
        if (r != kMarkerFF) {
            throw std::runtime_error("after dqt not ff");
        }
        if (!Controller()) {
            DLOG(ERROR) << "prethrow dqt no marker : " << static_cast<int>(reader_.Peek());
            throw std::runtime_error("after dqt no marker :(");
        }
    }

    void SOSF0Start() {
        uint16_t sz = reader_.ReadBits();
        uint16_t b = reader_.ReadBits();
        sz = (sz << 8) + b;
        DLOG(INFO) << "SOSf0 size : " << sz;
        std::vector<uint8_t> matrix;
        for (uint16_t i = 0; i + 2 < sz; ++i) {
            if (reader_.IsEof()) {
                DLOG(ERROR) << " SOSf0 EOF " << i + 1;
                throw std::runtime_error("SOSF0 end");
            }
            uint16_t value = reader_.ReadBits();
            matrix.push_back(value);
        }
        DLOG(INFO) << "SOSf0 MATRIX SIZE: " << matrix.size();
        meta_ = matrix;
        uint8_t r = reader_.ReadBits();
        if (r != kMarkerFF) {
            throw std::runtime_error("after sosf0 not ff");
        }
        if (!Controller()) {
            DLOG(ERROR) << "prethrow sosfo no marker : " << static_cast<int>(reader_.Peek());
            throw std::runtime_error("after dqt no marker :(");
        }
    }

    void DHTStart() {
        uint16_t sz = reader_.ReadBits();
        uint16_t b = reader_.ReadBits();
        sz = (sz << 8) + b;
        DLOG(INFO) << "DHT size : " << sz;
        sz -= 2;
        uint16_t i = 0;
        while (i < sz) {
            uint8_t ident = reader_.ReadBits();
            uint8_t number = ident & 15;
            uint8_t type_tree = (ident >> 4) & 15;
            cnts["DHT"]++;
            i++;
            DLOG(INFO) << "DHT num + typetree : " << static_cast<int>(number) << " "
                       << static_cast<int>(type_tree);
            std::vector<uint8_t> matrix;
            uint16_t sum = 0;
            for (uint16_t j = 0; j < 16; ++j) {
                if (reader_.IsEof()) {
                    DLOG(ERROR) << " DHT EOF " << i + 1;
                    throw std::runtime_error("DHT end");
                }
                uint8_t value = reader_.ReadBits();
                sum += value;
                matrix.push_back(value);
                ++i;
            }
            for (uint16_t j = 0; j < sum; ++j) {
                // DLOG(INFO) << " KEK ";
                if (reader_.IsEof()) {
                    DLOG(ERROR) << " DHT EOF " << i + 1;
                    throw std::runtime_error("DHT end");
                }
                uint8_t value = reader_.ReadBits();
                matrix.push_back(value);
                ++i;
            }
            DLOG(INFO) << "DHT MATRIX SIZE: " << matrix.size();
            if (type_tree == 0) {  //
                dht_dc_[number] = matrix;
            } else {
                dht_ac_[number] = matrix;
            }
        }
        if (i > sz) {
            throw std::runtime_error("dht problems");
        }
        uint8_t r = reader_.ReadBits();
        if (r != kMarkerFF) {
            throw std::runtime_error("after dht not ff");
        }
        if (!Controller()) {
            DLOG(ERROR) << "prethrow dht no marker : " << static_cast<int>(reader_.Peek());
            throw std::runtime_error("after dht no marker :(");
        }
    }

    void SOSStart() {
        uint16_t sz = reader_.ReadBits();
        uint16_t b = reader_.ReadBits();
        sz = (sz << 8) + b;
        DLOG(INFO) << "SOS size : " << sz;
        std::vector<uint8_t> matrix;
        for (uint16_t i = 0; i + 2 < sz; ++i) {
            if (reader_.IsEof()) {
                DLOG(ERROR) << " SOS EOF " << i + 1;
                throw std::runtime_error("SOS end");
            }
            uint16_t value = reader_.ReadBits();
            matrix.push_back(value);
        }
        if (matrix.back()) {
            throw std::runtime_error("invalid progressive");
        }
        matrix.pop_back();
        if (matrix.back() != 63) {
            throw std::runtime_error("invalid progressive");
        }
        matrix.pop_back();
        if (matrix.back()) {
            throw std::runtime_error("invalid progressive");
        }
        DLOG(INFO) << "SOS MATRIX SIZE: " << matrix.size();
        meta_mcu_ = matrix;
        matrix.clear();
        while (!reader_.IsEof()) {
            uint8_t t = reader_.ReadBits();
            if (t == kMarkerFF) {
                // DLOG(INFO) << "MEM" << " " << (int)reader_.Peek();
                if (reader_.Peek() == 0 && !reader_.IsEof()) {
                    reader_.ReadBits();
                    matrix.push_back(t);
                } else {
                    /*if (reader_.Peek() != kMarkerEOI) {
                        throw std::runtime_error("end?");
                    }*/
                    if (Controller()) {  // ?
                        if (bytes_mcu_.empty()) {
                            bytes_mcu_ = matrix;
                        }
                        return;
                    }
                }
            } else {
                matrix.push_back(t);
            }
        }
        Controller();
    }

public:
    std::map<std::string, int> cnts;
    std::map<uint8_t, std::vector<uint16_t>> dqts_;
    std::map<uint8_t, std::vector<uint8_t>> dht_dc_, dht_ac_;
    std::vector<uint8_t> comment_;
    std::vector<uint8_t> meta_;
    std::vector<uint8_t> meta_mcu_;
    std::vector<uint8_t> bytes_mcu_;
    // const size_t ksizeofbyte_ = 8;
    Readed<uint8_t, std::istream> reader_;
};
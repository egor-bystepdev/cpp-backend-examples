#include "fft.h"

#include <fftw3.h>
#include <stdexcept>
#include "fftw3.h"
#include <cmath>
#include <glog/logging.h>

DctCalculator::DctCalculator(size_t width, std::vector<double> *input,
                             std::vector<double> *output) {
    if (input->size() != output->size() || width * width != input->size()) {
        DLOG(ERROR) << "fftw error : " << input->size() << " " << output->size() << " "
                    << width * width;
        throw std::invalid_argument("inv");
    }
    input_ = input;
    output_ = output;
    width_ = width;
}

void DctCalculator::Inverse() {
    fftw_plan p;
    for (size_t i = 0; i < width_ * width_; ++i) {
        if (i % width_ == 0) {
            input_->data()[i] *= std::sqrt(2);
        }
        if (i / width_ == 0) {
            input_->data()[i] *= std::sqrt(2);
        }
    }
    p = fftw_plan_r2r_2d(width_, width_, input_->data(), output_->data(), FFTW_REDFT01,
                         FFTW_REDFT01, FFTW_ESTIMATE);
    fftw_execute(p);
    for (size_t i = 0; i < width_ * width_; ++i) {
        output_->data()[i] /= 16;
    }
    fftw_destroy_plan(p);
}

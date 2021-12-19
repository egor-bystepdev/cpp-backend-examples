#include "huffman.h"
#include <memory>
#include <stdexcept>

void HuffmanTree::Build(const std::vector<uint8_t> &code_lengths,
                        const std::vector<uint8_t> &values) {
    if (code_lengths.size() > 16) {
        throw std::invalid_argument("too big tree");
    }
    root_ = std::shared_ptr<HuffmanNode>(new HuffmanNode);
    move_ptr_ = root_;
    size_t head_ptr = 0;
    size_t i = 0;
    for (auto code : code_lengths) {
        i++;
        while (code--) {
            if (head_ptr == values.size()) {
                throw std::invalid_argument("values > code_length");
            }
            if (!DFS(root_, i, values[head_ptr++])) {
                throw std::invalid_argument("can't add value");
            }
        }
    }
    if (head_ptr != values.size()) {
        throw std::invalid_argument("values < code_length");
    }
    /*if (!CheckingCorectness(root_)) {
        throw std::invalid_argument("incorrect tree");
    }*/
}

bool HuffmanTree::Move(bool bit, int &value) {
    if (!move_ptr_) {
        throw std::invalid_argument("nullptr :)");
    }
    if (bit != 0 && bit != 1) {
        throw std::invalid_argument("incorrect bit");
    }
    if (bit == 0) {
        if (move_ptr_->left) {
            move_ptr_ = move_ptr_->left;
            if (move_ptr_->terminated) {
                value = move_ptr_->value;
                move_ptr_ = root_;
                return true;
            }
            return false;
        }
        throw std::invalid_argument("too nullptr");
    }
    if (bit == 1) {
        if (move_ptr_->right) {
            move_ptr_ = move_ptr_->right;
            if (move_ptr_->terminated) {
                value = move_ptr_->value;
                move_ptr_ = root_;
                return true;
            }
            return false;
        }
        throw std::invalid_argument("too nullptr");
    }
    return false;
}

bool HuffmanTree::DFS(std::shared_ptr<HuffmanNode> vert, uint8_t ln, uint8_t value, uint8_t deep) {
    if (vert->terminated) {
        return false;
    }
    if (deep == ln) {
        vert->value = value;
        vert->terminated = true;
        return true;
    }
    if (!vert->left) {
        vert->left = std::shared_ptr<HuffmanNode>(new HuffmanNode());
    }
    if (DFS(vert->left, ln, value, deep + 1)) {
        return true;
    }
    if (!vert->right) {
        vert->right = std::shared_ptr<HuffmanNode>(new HuffmanNode());
    }
    if (DFS(vert->right, ln, value, deep + 1)) {
        return true;
    }
    return false;
}

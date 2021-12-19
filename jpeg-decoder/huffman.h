#pragma once

#include <vector>
#include <cstddef>
#include <cstdint>
#include <memory>

// HuffmanTree decoder for DHT section.

struct HuffmanNode {
    std::shared_ptr<HuffmanNode> left = nullptr;
    std::shared_ptr<HuffmanNode> right = nullptr;
    bool terminated = false;
    uint8_t value;
    HuffmanNode() {
    }
};

class HuffmanTree {
public:
    HuffmanTree() : root_(nullptr), move_ptr_(nullptr) {
    }
    // code_lengths is the array of size no more than 16 with number of
    // terminated nodes in the Huffman tree.
    // values are the values of the terminated nodes in the consecutive
    // level order.
    void Build(const std::vector<uint8_t>& code_lengths, const std::vector<uint8_t>& values);

    // Moves the state of the huffman tree by |bit|. If the node is terminated,
    // returns true and overwrites |value|. If it is intermediate, returns false
    // and value is unmodified.
    bool Move(bool bit, int& value);

private:
    std::shared_ptr<HuffmanNode> root_;
    std::shared_ptr<HuffmanNode> move_ptr_;

    bool DFS(std::shared_ptr<HuffmanNode> vert, uint8_t ln, uint8_t value, uint8_t deep = 0);
};

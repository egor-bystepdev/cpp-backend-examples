#include "lru_cache.h"
#include <iostream>

LruCache::LruCache(size_t max_size) : max_size_(max_size) {
}

void LruCache::Set(const std::string& key, const std::string& value) {
    if (umap_.contains(key)) {
        values_.erase(umap_[key]);
        values_.push_front({key, value});
        umap_[key] = values_.begin();
    } else {
        if (values_.size() >= max_size_) {
            auto it = values_.end();
            --it;
            umap_.erase(it->first);
            values_.erase(it);
        }
        values_.push_front({key, value});
        umap_[key] = values_.begin();
    }
}

bool LruCache::Get(const std::string& key, std::string* value) {
    if (umap_.contains(key)) {
        auto it = umap_[key];
        *value = it->second;
        values_.push_front(*it);
        values_.erase(it);
        umap_[key] = values_.begin();
        return true;
    }
    return false;
}

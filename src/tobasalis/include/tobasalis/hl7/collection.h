#pragma once

#include <stdexcept>
#include <memory>
#include <string>
#include <vector>

namespace tbs {
namespace hl7 {

template<class T>
class Collection
{
private:
    std::vector<std::shared_ptr<T>> _vector;

public:
    /// Add item at the back
    void add(std::shared_ptr<T> item)
    {
        _vector.push_back(item);
    }

    /// Add item at position
    void add(std::shared_ptr<T> item, int index)
    {
        auto vectorSize = _vector.size();

        if (index < vectorSize) {
            _vector[index] = item;
        }
        else
        {
            for (auto comIndex = vectorSize; comIndex < index; comIndex++)
            {
                auto newItem = std::make_shared<T>(item->encoding());
                newItem->value(std::string());
                _vector.push_back( newItem);
            }

            _vector.push_back(item);
        }
    }

    /// Add item at first location
    void putAtFirt(std::shared_ptr<T>& item)
    {
        _vector.insert(_vector.begin(), item);
    }

    std::vector<std::shared_ptr<T>>& vector()
    {
        return _vector;
    }

    const std::vector<std::shared_ptr<T>>& vector() const
    {
        return _vector;
    }

    size_t size()
    {
        return _vector.size();
    }

    const size_t size() const
    {
        return _vector.size();
    }

    std::shared_ptr<T>& operator[](size_t index)
    {
        if (index >= _vector.size()) {
            throw std::out_of_range("Index out of range");
        }
        return _vector[index];
    }

    const std::shared_ptr<T>& operator[](size_t index) const
    {
        if (index >= _vector.size()) {
            throw std::out_of_range("Index out of range");
        }
        return _vector[index];
    }

    std::shared_ptr<T>& at(size_t index)
    {
        if (index >= _vector.size()) {
            throw std::out_of_range("Index out of range");
        }

        return _vector[index];
    }

    const std::shared_ptr<T>& at(size_t index) const
    {
        if (index >= _vector.size()) {
            throw std::out_of_range("Index out of range");
        }

        return _vector[index];
    }

    void erase(size_t index)
    {
        _vector.erase(_vector.begin() + index);
    }
};


} // namespace hl7
} // namespace tbs

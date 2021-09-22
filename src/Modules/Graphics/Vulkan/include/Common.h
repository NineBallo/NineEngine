//
// Created by nineball on 7/16/21.
//

#ifndef NINEENGINE_COMMON_H
#define NINEENGINE_COMMON_H
#include <deque>
#include <functional>

struct DeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void push_function(std::function<void()>&& function) {
        deletors.push_back(function);
    }

    void flush() {
        // reverse iterate the deletion queue to execute all the functions
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
            (*it)(); //call the function
        }

        deletors.clear();
    }
};
#endif //NINEENGINE_COMMON_H

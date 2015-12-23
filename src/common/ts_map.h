#ifndef _TS_MAP_H
#define _TS_MAP_H

#include <pthread.h>
#include <map>

template <typename T_key, typename T_value> class ts_map :
    public std::map<T_key, T_value> {
public:
    ts_map();
    ~ts_map();
    void insert(std::pair<T_key, T_value> pair);
    int  insert(std::pair<T_key, T_value> pair,
        typename ts_map<T_key, T_value>::iterator * it);
    void erase(typename ts_map<T_key, T_value>::iterator it);
    typename ts_map<T_key, T_value>::iterator find(T_key & key);
    bool getval(T_key & key, T_value * val);
    bool fetch(T_key & key, T_value * val);

private:
    pthread_mutex_t      mMutex;
    pthread_mutexattr_t  mMutexAttr;
};

template <typename T_key, typename T_value>
    ts_map<T_key, T_value>::ts_map() {

    pthread_mutexattr_init(&mMutexAttr);
    pthread_mutex_init(&mMutex, &mMutexAttr);
}

template <typename T_key, typename T_value>
    ts_map<T_key, T_value>::~ts_map() {
}

template <typename T_key, typename T_value>
    void ts_map<T_key, T_value>::insert(std::pair<T_key, T_value> pair) {

    pthread_mutex_lock(&mMutex);
    std::map<T_key, T_value>::insert(pair);
    pthread_mutex_unlock(&mMutex);
}

template <typename T_key, typename T_value>
    int ts_map<T_key, T_value>::insert(std::pair<T_key, T_value> pair,
    typename ts_map<T_key, T_value>::iterator * it) {

    int ret = 0;

    pthread_mutex_lock(&mMutex);

    typename ts_map<T_key, T_value>::iterator it_tmp =
        std::map<T_key, T_value>::find(pair.first);
    if (it_tmp != std::map<T_key, T_value>::end()) {
        *it = it_tmp;
        ret = -1;
    }
    else {
        std::map<T_key, T_value>::insert(pair);
        it_tmp = std::map<T_key, T_value>::find(pair.first);
        *it = it_tmp;
        ret = 0;
    }
    pthread_mutex_unlock(&mMutex);

    return ret;
}

template <typename T_key, typename T_value>
    void ts_map<T_key, T_value>::erase(
        typename ts_map<T_key, T_value>::iterator it) {

    pthread_mutex_lock(&mMutex);
    std::map<T_key, T_value>::erase(it);
    pthread_mutex_unlock(&mMutex);
}

template <typename T_key, typename T_value>
    typename ts_map<T_key, T_value>::iterator
    ts_map<T_key, T_value>::find(T_key & key) {

    typename ts_map<T_key, T_value>::iterator it_tmp;

    pthread_mutex_lock(&mMutex);
    it_tmp = std::map<T_key, T_value>::find(key);
    pthread_mutex_unlock(&mMutex);

    return it_tmp;
}

template <typename T_key, typename T_value>
    bool ts_map<T_key, T_value>::getval(T_key & key, T_value * val) {

    typename ts_map<T_key, T_value>::iterator it;

    pthread_mutex_lock(&mMutex);
    it = std::map<T_key, T_value>::find(key);
    if (it == std::map<T_key, T_value>::end()) {
        pthread_mutex_unlock(&mMutex);
        return false;
    }
    *val = it->second;
    pthread_mutex_unlock(&mMutex);

    return true;
}

/**
 * Find & Erase
 */
template <typename T_key, typename T_value>
    bool ts_map<T_key, T_value>::fetch(T_key & key, T_value * val) {

    typename ts_map<T_key, T_value>::iterator it_tmp;

    pthread_mutex_lock(&mMutex);
    it_tmp = std::map<T_key, T_value>::find(key);
    if (it_tmp == std::map<T_key, T_value>::end()) {
        pthread_mutex_unlock(&mMutex);
        return false;
    }
    *val = it_tmp->second;
    std::map<T_key, T_value>::erase(it_tmp);
    pthread_mutex_unlock(&mMutex);

    return true;
}


#endif


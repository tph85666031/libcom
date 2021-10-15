#include "com_kv_result.h"

KVResult::KVResult(const char* key, uint8_t* data, int dataSize)
{
    this->data = NULL;
    this->data_size = 0;
    if (key != NULL && data != NULL && dataSize > 0)
    {
        this->key = key;
        this->data = new uint8_t[dataSize]();
        this->data_size = dataSize;
        memcpy(this->data, data, dataSize);
    }
}

KVResult::KVResult(const KVResult& result)
{
    if (this->data != NULL)
    {
        delete[] this->data;
    }
    this->key = result.key;
    this->data = NULL;
    this->data_size = 0;

    if (result.data != NULL && result.data_size > 0)
    {
        this->data = new uint8_t[result.data_size]();
        this->data_size = result.data_size;
        memcpy(this->data, result.data, result.data_size);
    }
}

KVResult& KVResult::operator=(const KVResult& result)
{
    if (this != &result)
    {
        if (this->data != NULL)
        {
            delete[] this->data;
        }

        this->key = result.key;
        this->data = NULL;
        this->data_size = 0;


        if (result.data != NULL && result.data_size > 0)
        {
            this->data = new uint8_t[result.data_size]();
            this->data_size = result.data_size;
            memcpy(this->data, result.data, result.data_size);
        }
    }
    return *this;
}

KVResult::~KVResult()
{
    if (data != NULL)
    {
        delete[] data;
        data = NULL;
    }
}

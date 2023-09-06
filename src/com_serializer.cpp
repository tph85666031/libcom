#include <iostream>
#include "com_serializer.h"
#include "com_log.h"

Serializer::Serializer()
{
    this->pos_detach = 0;
}

Serializer::Serializer(const uint8* data, int data_size)
{
    load(data, data_size);
}

Serializer::~Serializer()
{
}

void Serializer::load(const uint8* data, int data_size)
{
    this->pos_detach = 0;
    if(data == NULL || data_size <= 0)
    {
        return;
    }
    for(int i = 0; i < data_size; i++)
    {
        buf.push_back(data[i]);
    }
}

Serializer& Serializer::enableByteOrderModify()
{
    this->change_byte_order = true;
    return *this;
}

Serializer& Serializer::disableByteOrderModify()
{
    this->change_byte_order = false;
    return *this;
}

Serializer& Serializer::append(bool val)
{
    buf.push_back((uint8)val);
    return *this;
}

Serializer& Serializer::append(char val)
{
    buf.push_back((uint8)val);
    return *this;
}

Serializer& Serializer::append(uint8 val)
{
    buf.push_back(val);
    return *this;
}

Serializer& Serializer::append(int8 val)
{
    buf.push_back((uint8)val);
    return *this;
}

Serializer& Serializer::append(uint16 val)
{
    if(change_byte_order)
    {
        val = htons(val);
    }
    uint8* p = (uint8*)&val;
    buf.push_back(p[0]);
    buf.push_back(p[1]);
    return *this;
}

Serializer& Serializer::append(int16 val)
{
    return append((uint16)val);
}

Serializer& Serializer::append(uint32 val)
{
    if(change_byte_order)
    {
        val = htonl(val);
    }
    uint8* p = (uint8*)&val;
    buf.push_back(p[0]);
    buf.push_back(p[1]);
    buf.push_back(p[2]);
    buf.push_back(p[3]);
    return *this;
}

Serializer& Serializer::append(int32 val)
{
    return append((uint32)val);
}

Serializer& Serializer::append(uint64 val)
{
    if(change_byte_order)
    {
        val = htonll(val);
    }
    uint8* p = (uint8*)&val;
    buf.push_back(p[0]);
    buf.push_back(p[1]);
    buf.push_back(p[2]);
    buf.push_back(p[3]);
    buf.push_back(p[4]);
    buf.push_back(p[5]);
    buf.push_back(p[6]);
    buf.push_back(p[7]);
    return *this;
}

Serializer& Serializer::append(int64 val)
{
    return append((uint64)val);
}

Serializer& Serializer::append(const char* val, int val_size)
{
    if(val_size < 0)
    {
        val_size = (int)strlen(val);
    }
    if(val != NULL && val_size > 0)
    {
        for(int i = 0; i < val_size; i++)
        {
            buf.push_back((uint8)val[i]);
        }
    }
    return *this;
}

Serializer& Serializer::append(const uint8* val, int val_size)
{
    if(val != NULL && val_size > 0)
    {
        for(int i = 0; i < val_size; i++)
        {
            buf.push_back(val[i]);
        }
    }
    return *this;
}

Serializer& Serializer::append(const std::string val, int val_size)
{
    if(val_size < 0)
    {
        val_size = (int)val.length();
    }
    append(val.c_str(), (int)val.length());
    return *this;
}

Serializer& Serializer::append(CPPBytes& bytes)
{
    append(bytes.getData(), bytes.getDataSize());
    return *this;
}

int Serializer::detach(bool& val)
{
    return detach((uint8&)val);
}

int Serializer::detach(char& val)
{
    return detach((uint8&)val);
}

int Serializer::detach(uint8& val)
{
    if(pos_detach >= (int)buf.size())
    {
        return -1;
    }
    val = buf[pos_detach];
    pos_detach += sizeof(uint8);
    return 0;
}

int Serializer::detach(int8& val)
{
    return detach((uint8&)val);
}

int Serializer::detach(uint16& val)
{
    if(pos_detach + sizeof(uint16) > buf.size())
    {
        return -1;
    }
    uint8* p = (uint8*)&val;
    p[0] = buf[pos_detach];
    p[1] = buf[pos_detach + 1];
    if(change_byte_order)
    {
        val = ntohs(val);
    }
    pos_detach += sizeof(uint16);
    return 0;
}

int Serializer::detach(int16& val)
{
    return detach((uint16&)val);
}

int Serializer::detach(uint32& val)
{
    if(pos_detach + sizeof(uint32) > buf.size())
    {
        return -1;
    }
    uint8* p = (uint8*)&val;
    p[0] = buf[pos_detach];
    p[1] = buf[pos_detach + 1];
    p[2] = buf[pos_detach + 2];
    p[3] = buf[pos_detach + 3];
    if(change_byte_order)
    {
        val = ntohl(val);
    }
    pos_detach += sizeof(uint32);
    return 0;
}

int Serializer::detach(int32& val)
{
    return detach((uint32&)val);
}

int Serializer::detach(uint64& val)
{
    if(pos_detach + sizeof(uint64) > buf.size())
    {
        return -1;
    }
    uint8* p = (uint8*)&val;
    p[0] = buf[pos_detach];
    p[1] = buf[pos_detach + 1];
    p[2] = buf[pos_detach + 2];
    p[3] = buf[pos_detach + 3];
    p[4] = buf[pos_detach + 4];
    p[5] = buf[pos_detach + 5];
    p[6] = buf[pos_detach + 6];
    p[7] = buf[pos_detach + 7];
    if(change_byte_order)
    {
        val = ntohll(val);
    }
    pos_detach += sizeof(uint64);
    return 0;
}

int Serializer::detach(int64& val)
{
    return detach((uint64&)val);
}

//读到\0结尾或val_size（val_size<0则读取到\0）
int Serializer::detach(std::string& val, int val_size)
{
    if(pos_detach >= (int)buf.size())
    {
        return -1;
    }

    if(val_size > 0)
    {
        if(pos_detach + val_size > (int)buf.size())
        {
            return -1;
        }
        val.assign((char*)&buf[pos_detach], val_size);
        pos_detach += val_size;
    }
    else
    {
        val.assign((char*)&buf[pos_detach]);
        pos_detach += val.length() + 1;
    }

    return 0;
}

//读到\0结尾或val_size
int Serializer::detach(char* val, int val_size)
{
    if(val == NULL || val_size <= 0)
    {
        return -1;
    }

    if(pos_detach >= (int)buf.size())
    {
        return -1;
    }

    strncpy(val, (char*)&buf[pos_detach], val_size);
    pos_detach += strlen(val) + 1;
    return 0;
}

//读val_size
int Serializer::detach(uint8* val, int val_size)
{
    if(val == NULL || val_size <= 0)
    {
        return -1;
    }

    if(pos_detach + val_size > (int)buf.size())
    {
        return -1;
    }

    memcpy(val, &buf[pos_detach], val_size);
    pos_detach += val_size;
    return 0;
}

//读val_size
int Serializer::detach(CPPBytes& bytes, int val_size)
{
    if(val_size <= 0)
    {
        return -1;
    }

    if(pos_detach + val_size > (int)buf.size())
    {
        return -1;
    }

    bytes.append(&buf[pos_detach], val_size);
    pos_detach += val_size;
    return 0;
}

void Serializer::clear()
{
    this->buf.clear();
    detachReset();
}

void Serializer::detachReset()
{
    this->pos_detach = 0;
}

int Serializer::getDetachRemainSize()
{
    return buf.size() - pos_detach;
}

CPPBytes Serializer::toBytes()
{
    CPPBytes bytes(&buf[0], buf.size());
    return bytes;
}

uint8* Serializer::getData()
{
    return &buf[0];
}

int Serializer::getDataSize()
{
    return buf.size();
}


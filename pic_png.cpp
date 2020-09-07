/*
MIT License

Copyright (c) 2020 SamuNatsu

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
Tutorial:
    Just use like this:
        #define PIC_PNG_IMPLEMENTATION
        #include "pic_png.h"

        int main() {
            PIC::PNG_CleanAndSave("Source.png", "Destination.png");
            return 0;
        }

    You SHOULD use "#define PIC_PNG_IMPLEMENTATION" at least once in any .cpp file for compiling.

    Or you can try other functions for fun, my code is very easy to understand :)
    
   
*/

#ifndef PIC_PNG_INCLUDED
#define PIC_PNG_INCLUDED

#include <cstdio>
#include <cstring>

#include <vector>

namespace PIC {

struct PNG_Chunk {
    unsigned int m_Size = 0x0;
    char m_Type[4] = {0x0, 0x0, 0x0, 0x0};
    unsigned char *m_Data = nullptr;
    unsigned int m_CRC = 0x0;
    PNG_Chunk() = default;
    PNG_Chunk(const PNG_Chunk&);
    ~PNG_Chunk();

    PNG_Chunk& operator=(const PNG_Chunk&);
};

bool PNG_CheckHeader(FILE*);

bool PNG_GetChunk(FILE*, PNG_Chunk&);

bool PNG_CleanAndSave(const char*, std::vector<PNG_Chunk>&);
bool PNG_CleanAndSave(const char*, const char*);

}

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef PIC_PNG_IMPLEMENTATION

namespace PIC {

PNG_Chunk::PNG_Chunk(const PNG_Chunk& tmp) {
    m_Size = tmp.m_Size;
    memcpy(m_Type, tmp.m_Type, 4);
    if (m_Size) {
        m_Data = new unsigned char[m_Size];
        memcpy(m_Data, tmp.m_Data, m_Size);
    }
    m_CRC = tmp.m_CRC;
}

PNG_Chunk::~PNG_Chunk() {
    if (m_Size)
        delete m_Data;
}

PNG_Chunk& PNG_Chunk::operator=(const PNG_Chunk& tmp) {
    if (m_Size)
        delete m_Data;
    m_Size = tmp.m_Size;
    memcpy(m_Type, tmp.m_Type, 4);
    if (m_Size) {
        m_Data = new unsigned char[m_Size];
        memcpy(m_Data, tmp.m_Data, m_Size);
    }
    m_CRC = tmp.m_CRC;
    return *this;
}

bool PNG_CheckHeader(FILE* fin) {
    const unsigned char s_Header[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    unsigned char _tmp[8];
    if (fread(_tmp, 1, 8, fin) != 8)
        return false;
    for (int i= 0; i < 8; ++i)
        if (s_Header[i] != _tmp[i])
            return false;
    return true;
}

bool PNG_GetChunk(FILE* fin, PNG_Chunk& chunk) {
    unsigned char _tmp[4];
    if (fread(_tmp, 1, 4, fin) != 4)
        return false;
    chunk.m_Size = 0x0;
    for (int i = 0, j = 24; i < 4; ++i, j -= 8)
        chunk.m_Size |= _tmp[i] << j;
    if (fread(chunk.m_Type, 1, 4, fin) != 4)
        return false;
    if (chunk.m_Size) {
        chunk.m_Data = new unsigned char[chunk.m_Size];
        if (fread(chunk.m_Data, 1, chunk.m_Size, fin) != chunk.m_Size)
            return false;
    }
    if (fread(_tmp, 1, 4, fin) != 4)
        return false;
    chunk.m_CRC = 0x0;
    for (int i = 0, j = 24; i < 4; ++i, j -= 8)
        chunk.m_CRC |= _tmp[i] << j;
    return true;
}

bool PNG_CleanAndSave(const char* path, std::vector<PNG_Chunk>& chunks) {
    FILE *fout = fopen(path, "wb");
    if (!fout) 
        return false;
    const unsigned char s_Header[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    if (fwrite(s_Header, 1, 8, fout) != 8) {
        fclose(fout);
        return false;
    }
    unsigned char _tmp[4];
    for (auto& i : chunks)
        if (i.m_Type[0] == 'I' || i.m_Type[0] == 'P') {
            for (int j = 0, k = 24; j < 4; ++j, k -= 8)
                _tmp[j] = (unsigned char)((i.m_Size >> k) & 0xFF);
            if (fwrite(_tmp, 1, 4, fout) != 4) {
                fclose(fout);
                return false;
            }
            if (fwrite(i.m_Type, 1, 4, fout) != 4) {
                fclose(fout);
                return false;
            }
            if (fwrite(i.m_Data, 1, i.m_Size, fout) != i.m_Size) {
                fclose(fout);
                return false;
            }
            for (int j = 0, k = 24; j < 4; ++j, k -= 8)
                _tmp[j] = (unsigned char)((i.m_CRC >> k) & 0xFF);
            if (fwrite(_tmp, 1, 4, fout) != 4) {
                fclose(fout);
                return false;
            }
        }
    return true;
}

bool PNG_CleanAndSave(const char* src, const char* dst) {
    FILE *fin = fopen(src, "rb");
    if (!fin)
        return false;
    if (!PNG_CheckHeader(fin)) {
        fclose(fin);
        return false;
    }
    PNG_Chunk _tmp;
    std::vector<PNG_Chunk> _chunks;
    while (!feof(fin)) {
        if (!PNG_GetChunk(fin, _tmp)) {
            if (feof(fin))
                break;
            fclose(fin);
            return false;
        }
        _chunks.emplace_back(_tmp);
        _tmp.m_Size = 0x0;
    }
    fclose(fin);
    if (!PNG_CleanAndSave(dst, _chunks))
        return false;
    return true;
}

}

#endif

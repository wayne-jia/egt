/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ERAWPARSE_H
#define ERAWPARSE_H


#include "eraw_define.h"
#include "../../../src/detail/erawimage.h"


class ImageParse
{
public:
    ImageParse(const char* filename, eraw_st* array_head, uint32_t array_size) noexcept
    {
        size_t buff_size = getFileSize(filename);
        void* buff_ptr = NULL;
        if (buff_size) 
        {
            buff_ptr = malloc(buff_size);
        } 
        else 
        {
            std::cerr << filename << " is blank!" << std::endl;
            return;
        }

        std::ifstream f(filename, std::ios::binary);
        if(!f)
        {
            std::cerr << "read " << filename << " failed!" << std::endl;
            free(buff_ptr);
            return;
        }
        
        f.read((char*)buff_ptr, buff_size);

        for (auto x = 0; x < array_size; x++)
            m_Images.push_back(egt::detail::ErawImage::load((const unsigned char*)buff_ptr+array_head[x].offset, array_head[x].len));

        free(buff_ptr);
    }

    egt::shared_cairo_surface_t GetImageObj(uint32_t index)
    {
        assert(index < m_Images.size());
        return m_Images[index];
    }

private:
    std::vector<egt::shared_cairo_surface_t> m_Images;
    size_t getFileSize(const char* fileName) 
    {
        if (fileName == NULL)
            return 0;

        struct stat statbuf;
        stat(fileName, &statbuf);
        return statbuf.st_size;
    }
};


#endif
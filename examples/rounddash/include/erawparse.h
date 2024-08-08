/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ERAWPARSE_H
#define ERAWPARSE_H

#include "eraw_define.h"
#include "../../../src/detail/erawimage.h"

/**
 * Convert the PNG/JPG/SVG... to *eraw.bin and read by program can make 
 * application boot fast, since there no need to decode PNG/JPG/SVG...
 * 
 * This class is to Parse the eraw.bin content to a list of cairo surface object.
 *
 * This is useful, for example, if you convert many png pictures by gfx-convert
 * tool and want to use the png in EGT, you should create an ImageParse instant
 * by ImageParse("path/to/eraw.bin", the_array_name, the_array_size); then in 
 * the code, you will get a shared_cairo_surface_t type vector which contains
 * a list of image objects. The object can be used directly by egt::Image(object).
 *
 * @b Example
 * @code{.cpp}
 * ImageParse imgs("stage1_eraw.bin", Speedo_table, sizeof(Speedo_table)/sizeof(eraw_st));
 * window.background(egt::Image(imgs.GetImageObj(0))); //Use the first image object as
 *                                                     //the background picture.
 * ...
 * @endcode
 */
class ImageParse
{
public:

    /**
     * @param[in] filename The *eraw.bin file name.
     * @param[in] array_head The array name in corresponding *eraw.h.
     * @param[in] array_size The array size in corresponding *eraw.h.
     */
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

    /**
     * @param[in] index The index of image vector.
     */
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
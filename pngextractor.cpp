// pngextractor.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

constexpr uint8_t png_header[]{ 137, 80, 78, 71, 13, 10, 26, 10 };
constexpr size_t minimum_png_size{ 20 };
constexpr auto chunk_end_type{ "IEND" };

template <typename T>
T big_endian_to_little_endian(const std::vector<uint8_t> & v)
{
    volatile T result{ 0 };
    if (v.size() < sizeof(T))
    {
        return result;
    }

    auto t_ptr = reinterpret_cast<volatile uint8_t *>(&result);
    for (size_t i = 0; i < sizeof(T); ++i)
    {
        t_ptr[sizeof(T) - (i + 1)] = v[i];
    }

    return result;
}

int main(int argc, char ** argv)
{
    if (argc < 2)
    {
        return 1;
    }

    std::string filename = argv[1];

    std::ifstream file{ filename, std::ios::in | std::ios::binary };
    const std::vector<uint8_t> filedata{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };

    auto png_start = filedata.cbegin();
    size_t png_counter{ 0 };
    do
    {
        png_start = std::search(png_start, filedata.cend(), png_header, png_header + sizeof(png_header));
        if (png_start != filedata.end())
        {
            std::cout << "found png header\n";

            std::cout << "\tanalyzing...\n";
            auto png_current = png_start;
            // check size
            if (std::distance(png_start, filedata.cend()) < minimum_png_size)
            {
                std::cout << "\ttoo small binary\n" << std::endl;
                break;
            }

            // first chunk
            auto chunk_start = png_current + sizeof(png_header);
            auto png_end = png_start;

            // read length
            std::string chunk_type{ "TEST" };
            do
            {
                auto chunk_it = chunk_start;
                auto chunk_length = big_endian_to_little_endian<uint32_t>(std::vector<uint8_t>{ chunk_it, chunk_it + sizeof(uint32_t) });
                std::cout << "\tchunk length: " << std::dec << chunk_length << std::endl;
                chunk_it += sizeof(chunk_length);
                chunk_type = std::string{ chunk_it, chunk_it + 4 };
                std::cout << "\tchunk type: " << chunk_type << std::endl;
                chunk_it += chunk_type.size();
                // skip chunk data
                chunk_it += static_cast<size_t>(chunk_length);

                auto chunk_crc = big_endian_to_little_endian<uint32_t>(std::vector<uint8_t>{ chunk_it, chunk_it + sizeof(uint32_t) });
                std::cout << "\tchunk crc: " << std::hex << chunk_crc << std::endl;
                chunk_it += sizeof(chunk_crc);
                chunk_start = chunk_it;
                png_end = chunk_it;
            } while (chunk_type != chunk_end_type);
            
            auto png_filename = "png_" + std::to_string(png_counter) + ".png";
            std::ofstream outfile(png_filename, std::ios::out | std::ios::binary);
            const std::vector<uint8_t> png_data{ png_start, png_end };
            std::copy(png_data.cbegin(), png_data.cend(), std::ostreambuf_iterator<char>(outfile));
            std::cout << "File " << png_filename << " written!\n\n" << std::endl;

            // goto next
            ++png_start;
            ++png_counter;
        }
    } while (png_start != filedata.end());

    return 0;
}


#include "terrain.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>

#include <core/Log.h>


using namespace terrain;

const uint32_t DEM_BLOCK_SIZE { 1024 };

using DEM_block = std::array<char, DEM_BLOCK_SIZE>;
using DEM_blocks = std::vector<DEM_block>;


int32_t parseInt(const char* c) {
    std::string s(c, 6);
    return std::stoi(s);
}

float parseReal4(const char* c) {
    std::string s(c, 12);
    std::replace(s.begin(), s.end(), 'D', 'E');
    return std::stof(s);
}

float parseReal8(const char* c) {
    std::string s(c, 24);
    std::replace(s.begin(), s.end(), 'D', 'E');
    return std::stof(s);
}


struct DEMHeader {
    DEM_block& _block;
    
    uint32_t number_columns() const { 
        return parseInt(&_block[858]);
    };

    float elevation_unit_scale() const {
        auto unit = parseInt(&_block[534]);
        if (unit == 1) {
            return 0.33f;
        } else {
            return 1.0f;
        }
    }

    float zero_elevation() const {
        auto unit = parseInt(&_block[534]);
        if (unit == 1) {
            return 0.33f;
        }
        else {
            return 1.0f;
        }
    }

    float Z_spatial_resolution() const {
        return parseReal4(&_block[840]);
    };
};

struct DEMProfile {
    DEM_blocks::iterator _block_it;

    std::string header() const {
        std::string s((*_block_it).data(), 144);
        return s;
    }

    uint32_t number_rows() const {
        std::string s(&(*_block_it)[12], 18 - 12);
        return std::stoi(s);
    };

    uint32_t number_extra_blocks() const {
        uint32_t num_rows = number_rows();
        uint32_t num_values = num_rows * 1;

        // a block is 1024 B 
        // a value is 6B
        // there are 4B white chars at the end of the block

        // first profile block can hold 146 values
        // (1024 - 144 -4) / 6 = 146
        if (num_values <= 146) {
            return 0;
        }

        // then 170 per extra blocks
        // (1024 - 4) / 6 = 170
        num_values -= 146;
        return (num_values / 170) + ((num_values % 170) == 0 ? 0 : 1);
    }

    std::stringstream make_data_stream(const DEM_blocks::iterator& first, uint32_t num_blocks, const DEM_blocks::iterator& end) const {
        auto it = first;
        std::string buffer = std::string((*first).data() + 144, DEM_BLOCK_SIZE - 144);

        for (int b = 0; b < num_blocks; b++) {
            it++;
            if (it == end) {
                break;
            }
            buffer += std::string((*it).data(), DEM_BLOCK_SIZE);
        }

        std::stringstream data_stream(buffer);
        return data_stream;
    }


    float local_datum_elevation() const {
        return parseReal8(&(*_block_it)[24]);
    }

    core::vec2 elevation_min_max() const {
        return core::vec2(parseReal8(&(*_block_it)[96]), parseReal8(&(*_block_it)[120]));
    }

    DEMProfile(const DEM_blocks::iterator& first) :
        _block_it(first) {
    }
};
using DEM_profiles = std::vector<DEMProfile>;

TerrainPointer Terrain::createFromDEM(const std::string& filename) {
    if (filename.empty()) {
        return nullptr;
    }
    auto dem_path = std::filesystem::absolute(std::filesystem::path(filename));
    if (!std::filesystem::exists(dem_path)) {
        picoLog("meta file " + dem_path.string() + " doesn't exist");
        return nullptr;
    }


    // let's try to open the file
    std::ifstream file(filename, std::ifstream::in);
    std::stringstream asciiStream;
    asciiStream << file.rdbuf();
    file.close();

    DEM_blocks dem_blocks;
    while (!asciiStream.eof()) {
        dem_blocks.emplace_back();
        asciiStream.read(dem_blocks.back().data(), DEM_BLOCK_SIZE);
    }

    uint32_t current_block_idx = 0;
    auto current_block_it = dem_blocks.begin();
    DEMHeader header = { *current_block_it };
    current_block_it++;
    current_block_idx++;
    auto num_columns = header.number_columns();
    auto z_spatial_resolution = header.Z_spatial_resolution();

    // parse the profiles and evaluate max num rows
    DEM_profiles dem_profiles;
    uint32_t dem_num_rows = 1;
    

    for (uint32_t i = 0; i < num_columns; i++) {

        auto& profile = dem_profiles.emplace_back( current_block_it );
        uint32_t num_rows = profile.number_rows();

        dem_num_rows = core::max(dem_num_rows, num_rows);

        uint32_t row_num_extra_blocks = profile.number_extra_blocks();
        current_block_it += row_num_extra_blocks;
        current_block_idx += row_num_extra_blocks;

        if (current_block_idx >= dem_blocks.size()) {
            std::cout << " PROBLEM " << std::endl;
            break;
        }

        current_block_it += 1;
        current_block_idx += 1;
    }

    auto terrain = std::make_shared<Terrain>(core::max(num_columns, dem_num_rows), 10.0f);

    uint32_t profile_num = 0;
    auto zero_datum_height = 0.0f;

    for (const auto& profile : dem_profiles) {
        
        uint32_t num_rows = profile.number_rows();
        uint32_t row_num_extra_blocks = profile.number_extra_blocks();
        auto data_stream = profile.make_data_stream(profile._block_it, row_num_extra_blocks, dem_blocks.end());
        
        auto height_min_max = profile.elevation_min_max();
        auto local_datum_height = profile.local_datum_elevation();
        if (profile_num == 0){
            zero_datum_height = local_datum_height;
            local_datum_height = 0.0f;
        } else {
            local_datum_height -= zero_datum_height;
        }

        if (!data_stream.eof()) {
            std::string token;
            for (int j = 0; j < num_rows; j++) {
                data_stream >> token;
                auto v = std::stoi(token);
                if (v != 0) {
                    terrain->_heights[profile_num * dem_num_rows + j] = /*local_datum_height*/ + (std::stoi(token)) * z_spatial_resolution;
                   // terrain->_heights[profile_num * dem_num_rows + j] = local_datum_height + (std::stoi(token) / 255.f) * (height_min_max.y - height_min_max.x) + height_min_max.x;
                    
                }
                else {
                    terrain->_heights[profile_num * dem_num_rows + j] = local_datum_height;

                }
            }
        }
        else {
            std::cout << " PROBLEM " << std::endl;
            break;
        }
        profile_num++;
    }


    return terrain;
}

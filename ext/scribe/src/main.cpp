//
//  main.cpp
//  tools/shaderScribe/src
//
//  Created by Sam Gateau on 12/15/2014.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


#include "TextTemplate.h"

#include <fstream>
#include <sstream>
#include <ctime>
#include <chrono>
#include <format>

using namespace std;

int main (int argc, char** argv) {
    // process the command line arguments
    std::vector< std::string > inputs;

    std::string srcFilename;
    std::string destFilename;
    std::string targetName;
    std::string targetExt;
    std::list<std::string> headerFiles;
    TextTemplate::Vars vars;
    
    std::string lastVarName;
    bool listVars = false;
    bool makefileDeps = false;
    bool showParseTree = false;
    bool makeCPlusPlus = false;
    bool parseShaderTypeFromFilename = false;
    bool makeResourceDeclaration = false;

    auto config = std::make_shared<TextTemplate::Config>();

    enum Mode {
        READY = 0,
        GRAB_OUTPUT,
        GRAB_VAR_NAME,
        GRAB_VAR_VALUE,
        GRAB_INCLUDE_PATH,
        GRAB_TARGET_NAME,
        GRAB_SHADER_TYPE,
        GRAB_HEADER,
        EXIT,
    } mode = READY;

    enum Type {
        VERTEX = 0,
        FRAGMENT,
        COMPUTE,
        INCLUDE,
    } type = VERTEX;
    static const char* shaderTypeString[] = {
        "VERTEX", "PIXEL", "COMPUTE", "INCLUDE"
    };
    static const char* shaderCreateString[] = {
        "Vertex", "Pixel", "Compute", "Include"
    };
    std::string shaderStage{ "vert" };

    auto shaderStageParser = [&](const std::string& token) {
        if (token == "frag") {
            shaderStage = token;
            type = FRAGMENT;
        } else if (token == "comp") {
            shaderStage = token;
            type = COMPUTE;
        } else if (token == "vert") {
            shaderStage = token;
            type = VERTEX;
        } else if (token == "inc") {
            shaderStage = token;
            type = INCLUDE;
        } else {
            cerr << "Unrecognized shader type. Supported is vert, frag, comp or inc" << endl;
        }
    };

    for (int ii = 1; (mode != EXIT) && (ii < argc); ii++) {
        inputs.push_back(argv[ii]);

        switch (mode) {
            case READY: {
                if (inputs.back() == "-o") {
                    mode = GRAB_OUTPUT;
                } else if (inputs.back() == "-H") {
                    mode = GRAB_HEADER;
                } else if (inputs.back() == "-t") {
                    mode = GRAB_TARGET_NAME;
                } else if (inputs.back() == "-D") {
                    mode = GRAB_VAR_NAME;
                } else if (inputs.back() == "-I") {
                    mode = GRAB_INCLUDE_PATH;
                } else if (inputs.back() == "-listVars") {
                    listVars = true;
                    mode = READY;
                } else if (inputs.back() == "-M") {
                    makefileDeps = true;
                    mode = READY;
                } else if (inputs.back() == "-showParseTree") {
                    showParseTree = true;
                    mode = READY;
                } else if (inputs.back() == "-c++") {
                    makeCPlusPlus = true;
                    makeResourceDeclaration = true;
                    mode = READY;
                } else if (inputs.back() == "-T") {
                    mode = GRAB_SHADER_TYPE;
                } else if (inputs.back() == "-parseFilenameType") {
                    parseShaderTypeFromFilename = true;
                    mode = READY;
                } else {
                    // just grabbed the source filename, stop parameter parsing
                    srcFilename = inputs.back();
                    mode = READY;
                }
            }
            break;

            case GRAB_OUTPUT: {
                destFilename = inputs.back();
                mode = READY;
            }
            break;

            case GRAB_TARGET_NAME: {
                targetName = inputs.back();
                mode = READY;
            }
            break;

            case GRAB_HEADER: {
                headerFiles.push_back(inputs.back());
                mode = READY;
            }
            break;

            case GRAB_VAR_NAME: {
                // grab first the name of the var
                lastVarName = inputs.back();
                mode = GRAB_VAR_VALUE;
            }
            break;
            case GRAB_VAR_VALUE: {
                // and then the value
                vars.insert(TextTemplate::Vars::value_type(lastVarName, inputs.back()));

                mode = READY;
            }
            break;

            case GRAB_INCLUDE_PATH: {
                config->addIncludePath(inputs.back().c_str());
                mode = READY;
            }
            break;
                
            case GRAB_SHADER_TYPE:
            {
                shaderStageParser(inputs.back());
                mode = READY;
            }
            break;

            case EXIT: {
                // THis shouldn't happen
            }
            break;
        }
    }

    if (srcFilename.empty()) {
        cerr << "Usage: shaderScribe [OPTION]... inputFilename" << endl;
        cerr << "Where options include:" << endl;
        cerr << "  -o filename: Send output to filename rather than standard output." << endl;
        cerr << "  -t targetName: Set the targetName used, if not defined use the output filename 'name' and if not defined use the inputFilename 'name'" << endl;
        cerr << "  -I include_directory: Declare a directory to be added to the includes search pool." << endl;
        cerr << "  -D varname varvalue: Declare a var used to generate the output file." << endl;
        cerr << "       varname and varvalue must be made of alpha numerical characters with no spaces." << endl;
        cerr << "  -H : Prepend the contents of header file to the scribe output " << endl;
        cerr << "       This can be specified multiple times and the headers will be applied in the specified order" << endl;
        cerr << "  -M : Emit a list of files that the scribe output depends on, for make and similar build tools " << endl;
        cerr << "  -listVars : Will list the vars name and value in the standard output." << endl;
        cerr << "  -showParseTree : Draw the tree obtained while parsing the source" << endl;
        cerr << "  -c++ : Generate a c++ source file containing the output file stream stored as a char[] variable" << endl;
        cerr << "  -T vert/frag/comp : define the type of the shader. Defaults to VERTEX if not specified." << endl;
        cerr << "       This is necessary if the -c++ option is used." << endl;
        cerr << "  -parseFilenameType : the shader type is deduced from the filename." << endl;
        cerr << "       nameOfShaderFile.vert.[language extension] => Vertex" << endl;
        cerr << "       nameOfShaderFile.frag.[language extension] => Fragment" << endl;
        cerr << "       nameOfShaderFile.comp.[language extension] => Compute" << endl;
        cerr << "       nameOfShaderFile.inc.[language extension] => Include" << endl;
        cerr << "       This is necessary if the -c++ option is used." << endl;

        return 0;
    }

    // Define targetName: if none, get destFilename, if none get srcFilename
    if (targetName.empty()) {
        if (destFilename.empty()) {
            targetName = srcFilename;
        } else {
            targetName = destFilename;
        }
    }

    if (parseShaderTypeFromFilename) {
        // trim anything before '/' or '\'
        auto shaderTypeToken = srcFilename.substr(srcFilename.find_last_of('/') + 1);
        shaderTypeToken = shaderTypeToken.substr(shaderTypeToken.find_last_of('\\') + 1);

        // Get the last extesntion from the input has the target
        targetExt = shaderTypeToken.substr(shaderTypeToken.find_last_of('.'));

        // trim anything after '.'
        shaderTypeToken = shaderTypeToken.substr(0, shaderTypeToken.find_first_of('.'));

        // now extract the expected type name at the end of the reminaining token after '_'
        shaderTypeToken = shaderTypeToken.substr(shaderTypeToken.find_last_of('_') + 1);
        shaderStageParser(shaderTypeToken);
    }

    // no clean it to have just a descent c var name
    if (!targetName.empty()) {
        // trim anything before '/' or '\'
        targetName = targetName.substr(targetName.find_last_of('/') + 1);
        targetName = targetName.substr(targetName.find_last_of('\\') + 1);

        // trim anything after last '.'
        targetName = targetName.substr(0, targetName.find_first_of('.'));
    }

    // Add built in vars
    time_t endTime = chrono::system_clock::to_time_t(chrono::system_clock::now());
    std::string endTimStr(ctime(&endTime));
    vars["_SCRIBE_DATE"] = endTimStr.substr(0, endTimStr.length() - 1);

    // List vars?
    if (listVars) {
        cerr << "Vars:" << endl;
        for (auto v : vars) {
            cerr << "    " << v.first << " = " << v.second << endl;
        }
    }

    // Open up source
    std::fstream srcStream;
    srcStream.open(srcFilename, std::fstream::in);
    if (!srcStream.is_open()) {
        cerr << "Failed to open source file <" << srcFilename << ">" << endl;
        return 1;
    }

    auto scribe = std::make_shared<TextTemplate>(srcFilename, config);

    std::string header;
    if (!headerFiles.empty()) {
        for (const auto& headerFile : headerFiles) {
            std::fstream headerStream;
            headerStream.open(headerFile, std::fstream::in);
            if (!headerStream.is_open()) {
                cerr << "Failed to open source file <" << headerFile << ">" << endl;
                return 1;
            }
            header += std::string((std::istreambuf_iterator<char>(headerStream)), std::istreambuf_iterator<char>());
            headerStream.close();
        }
    }

    // Add the type define to the shader
    switch (type) {
    case VERTEX:
        header += "#define GPU_VERTEX_SHADER\n";
        break;

    case FRAGMENT:
        header += "#define GPU_PIXEL_SHADER\n";
        break;

    case COMPUTE:
        header += "#define GPU_COMPUTE_SHADER\n";
        break;
    }

    // ready to parse and generate
    std::stringstream destStringStream;
    // first header
    destStringStream << header;
    // after header, reset line count
    destStringStream << "#line 0\n";
    // now parse
    int numErrors = scribe->scribe(destStringStream, srcStream, vars);
    // Close srcStream, not needed anymore
    srcStream.close();
    if (numErrors) {
        cerr << "Scribe " << srcFilename << "> failed: " << numErrors << " errors." << endl;
        return 1;
    };

    std::stringstream destStringStreamBis(destStringStream.str());


    if (showParseTree) {
        int level = 1;
        cerr << "Config trees:" << std::endl;
        config->displayTree(cerr, level);

        cerr << srcFilename << " tree:" << std::endl;
        scribe->displayTree(cerr, level);
    }

    // after the source has been generated, do a new pass to identify shader resource
    struct IncludeDesc {
        std::string name;
    };
    std::vector<IncludeDesc> includeDescs;
    struct StructDesc {
        struct Attribute {
            std::string type;
            std::string member;
        };
        std::string name;
        std::vector<Attribute> attributes;
        std::string block;
    };
    std::vector<StructDesc> structDescs;
    struct ResourceDesc {
        std::string name;
        std::string desc;
        std::string registerslot;
        std::string type;
    };
    std::vector<ResourceDesc> pushDescs;
    std::vector<ResourceDesc> resourceDescs;
    if (makeResourceDeclaration)
    {
       // destStringStream.seekg(0);

        std::string lineToken;
        std::string block;

        enum class ParseMode {
            none = 0,
            block_begin = 1,
            block_end = 2,
        };
        ParseMode parseMode = ParseMode::none;
        enum class TokenMode {
            none = 0,
            pushbuffer = 1,
            structdec = 2,
        };
        TokenMode tokenMode = TokenMode::none;
        

        int register_id = -1;
        bool isInCommentBlock = false;
        int32_t bracketBlockDepth = 0;
        while (std::getline(destStringStreamBis, lineToken)) {

            // check for comment block exit
            size_t commentEnd_idx = lineToken.find("*/");
            if (isInCommentBlock)
            {
                if (commentEnd_idx != std::string::npos)
                {
                    lineToken = lineToken.substr(commentEnd_idx);
                    isInCommentBlock = false;
                }
                else
                {
                    continue;
                }
            }

            size_t commentBegin_idx = lineToken.find("/*");
            if (commentBegin_idx != std::string::npos)
            {
                std::string before = lineToken.substr(0, commentBegin_idx);
                commentEnd_idx = lineToken.find("*/");
                if (commentEnd_idx != std::string::npos && commentEnd_idx > commentBegin_idx)
                {
                    std::string after = lineToken.substr(commentEnd_idx);
                    lineToken = before + after;
                }
                else
                {
                    lineToken = before;
                    isInCommentBlock = true;
                }                
            }

            // if incomment remove the line, move on
            if (isInCommentBlock)
            {
                continue;
            }

            // else commented line ?
            size_t commentLine_idx = lineToken.find("//");
            if (commentLine_idx != std::string::npos)
            {
                if (commentLine_idx > 0)
                {
                    lineToken = lineToken.substr(0, commentLine_idx);
                }
                else
                {
                    continue; //  just remove the line
                }
            }

            // else...

            size_t include_idx = lineToken.find("#include");
            size_t struct_idx = lineToken.find("struct ");
            size_t pushbuffer_idx = lineToken.find("cbuffer");
            size_t uniformbuffer_idx = lineToken.find("ConstantBuffer<");
            size_t rwstructuredbuffer_idx = lineToken.find("RWStructuredBuffer<");
            size_t rwbuffer_idx = lineToken.find("RWBuffer<");
            size_t structuredbuffer_idx = lineToken.find("StructuredBuffer<");
            size_t buffer_idx = lineToken.find("Buffer<");
            size_t samplerstate_idx = lineToken.find("SamplerState");
            size_t texture_idx = lineToken.find("Texture");

            size_t register_idx = lineToken.find(": register(");
            size_t register_begin_idx = std::string::npos;
            size_t register_end_idx = std::string::npos;
            if (register_idx != std::string::npos)
            {
                register_begin_idx = register_idx + 11;
                register_end_idx = lineToken.find(")", register_begin_idx);
            }

            size_t bracketOpen_idx = lineToken.find("{");
            size_t bracketClose_idx = lineToken.find("}");

            if (include_idx != std::string::npos)
            {
                size_t includeBegin_idx = lineToken.find("\"", 9);
                size_t includeEnd_idx = includeBegin_idx;
                if (includeBegin_idx == std::string::npos)
                {
                    includeBegin_idx = lineToken.find("<", 9) + 1;
                    includeEnd_idx = lineToken.find(">", includeBegin_idx);
                }
                else
                {
                    includeBegin_idx += 1;
                    includeEnd_idx = lineToken.find("\"", includeBegin_idx);
                }
                IncludeDesc id = {
                    lineToken.substr(includeBegin_idx, includeEnd_idx - includeBegin_idx - 5) // remove ".hlsl"
                };
                 includeDescs.emplace_back(id);
            }
            else if (struct_idx != std::string::npos)
            {
                int32_t l = std::string::npos;
                if (bracketOpen_idx != std::string::npos)
                    l = bracketOpen_idx - struct_idx - 8;
                StructDesc sd{
                    .name = lineToken.substr(struct_idx + 7, l),
                };
                structDescs.emplace_back(sd);
                tokenMode = TokenMode::structdec;
                parseMode = ParseMode::block_begin;
            }
            else if (pushbuffer_idx != std::string::npos) {
                pushbuffer_idx += 8; // begining of cbuffer name
                ResourceDesc rd{
                    .name = lineToken.substr(pushbuffer_idx, register_idx - 1 - pushbuffer_idx),
                    .desc = "graphics::DescriptorType::PUSH_UNIFORM",
                    .registerslot = lineToken.substr(register_begin_idx + 1, register_end_idx - register_begin_idx - 1) };
                pushDescs.emplace_back(rd);

                StructDesc sd{
                    .name = rd.name,
                };
                structDescs.emplace_back(sd);
                tokenMode = TokenMode::pushbuffer;
                parseMode = ParseMode::block_begin;
            }
            else if (uniformbuffer_idx != std::string::npos) {
                uniformbuffer_idx += 15; // begining of ConstantBuffer< type_name >
                size_t closing_idx = lineToken.find("> ");
                if (closing_idx == std::string::npos) {
                    continue;
                }
                ResourceDesc rd{
                    .name = lineToken.substr(closing_idx + 2, register_idx - 1 - closing_idx - 2),
                    .desc = "graphics::DescriptorType::UNIFORM_BUFFER",
                    .registerslot = lineToken.substr(register_begin_idx + 1, register_end_idx - register_begin_idx - 1),
                    .type = lineToken.substr(uniformbuffer_idx, closing_idx - uniformbuffer_idx),
                };
                resourceDescs.emplace_back(rd);
            }
            else if ((rwstructuredbuffer_idx != std::string::npos) || (rwbuffer_idx != std::string::npos)) {
                std::string resource = "{ graphics::DescriptorType::RW_RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 11, 1 }";
  //              resources.emplace_back(resource);
            }
            else if ((structuredbuffer_idx != std::string::npos) || (buffer_idx != std::string::npos)) {
                size_t closing_idx = lineToken.find("> ");
                if (closing_idx == std::string::npos) {
                    continue;
                }
                ResourceDesc rd{
                    .name = lineToken.substr(closing_idx + 2, register_idx - 1 - closing_idx - 2),
                    .desc = "graphics::DescriptorType::RESOURCE_BUFFER",
                    .registerslot = lineToken.substr(register_begin_idx + 1, register_end_idx - register_begin_idx - 1) };
                resourceDescs.emplace_back(rd);
            }
            else if (samplerstate_idx != std::string::npos) {
                std::string resource = " { graphics::DescriptorType::SAMPLER, graphics::ShaderStage::COMPUTE, 0, 2}";
            }
            else if (texture_idx != std::string::npos) {
                std::string resource = "{ graphics::DescriptorType::RESOURCE_TEXTURE, graphics::ShaderStage::VERTEX, 11, 1 }";
            }

            bool resource_parsed = false;
            if (parseMode == ParseMode::block_begin)
            {
                if (bracketOpen_idx != std::string::npos)
                {
                    if (bracketClose_idx != std::string::npos)
                    {
                        bracketClose_idx -= bracketOpen_idx + 2;
                        parseMode = ParseMode::none;
                        resource_parsed = true;
                    }
                    else
                    {
                        parseMode = ParseMode::block_end;
                    }
                    block = lineToken.substr(bracketOpen_idx + 1, bracketClose_idx);
                }
            }
            else if (parseMode == ParseMode::block_end) {
                if (bracketOpen_idx != std::string::npos)
                {
                    bracketBlockDepth++;
                    // if 
                    if (bracketBlockDepth <= 1)
                    {
                        if (bracketClose_idx != std::string::npos) {
                            auto ending = lineToken.substr(bracketClose_idx + 1, std::string::npos);
                            lineToken = lineToken.substr(0, bracketOpen_idx) + ending;
                            bracketBlockDepth--;
                        }
                        else                    
                            lineToken = lineToken.substr(0, bracketOpen_idx);
                    }
                    else
                        lineToken.clear();

                }
                else if (bracketClose_idx != std::string::npos)
                {
                    if (bracketBlockDepth > 0)
                    {
                        bracketBlockDepth--;
                        lineToken = lineToken.substr(bracketClose_idx + 1, -1);
                    }
                    else
                    {
                        if (bracketClose_idx > 0)
                            lineToken = lineToken.substr(0, bracketClose_idx);
                        else
                            lineToken.clear();
                        
                        parseMode = ParseMode::none;
                        resource_parsed = true;
                    }
                }
                

                if (bracketBlockDepth == 0 && !lineToken.empty())
                {
                    block += lineToken + "\n";
                }
               
            }

            if (resource_parsed)
            {
                switch (tokenMode)
                {
                case TokenMode::pushbuffer:
                case TokenMode::structdec:
                {
                    structDescs.back().block = block;
                    std::stringstream blockStream(block);
                    //blockStream >> std::ws;
                    std::string blockLineToken;
                    std::vector<std::string> lineTokens;
                    bool endOfCodeLine = false;
                    bool skipAsFunction = false;
                    while(blockStream >> blockLineToken)
                    {
                        if (!endOfCodeLine) {
                            size_t semicolumn_idx = blockLineToken.find(';');
                            size_t bracket_idx = blockLineToken.find(')');
                            if (semicolumn_idx != std::string::npos || bracket_idx != std::string::npos)
                            {
                                blockLineToken = blockLineToken.substr(0, semicolumn_idx);
                                endOfCodeLine = true;
                                if (bracket_idx != std::string::npos)
                                    skipAsFunction = true;
                            }
                            lineTokens.emplace_back(blockLineToken);
                        }

                        if (blockStream.peek() == '\n')
                        {
                            blockStream.ignore();
                            if (!skipAsFunction)
                            {
                                StructDesc::Attribute att{ lineTokens[0], lineTokens[1] };
                                structDescs.back().attributes.emplace_back(att);

                            }
                            lineTokens.clear();
                            endOfCodeLine = false;
                            skipAsFunction = false;
                        }
                    }

                    tokenMode = TokenMode::none;
                    block.clear();
                    register_id = -1;
                } break;
                
                };
            }
        };

        // Filter struct types from hlsl to cpp
        std::vector<StructDesc::Attribute> hlslTocppAliases = {
            { "int4",       "core::ivec4" },
            { "int",        "int32_t" },
            { "uint",        "uint32_t" },
            { "float4",     "core::vec4" },
            { "float3",     "core::vec3" },
            { "float2",     "core::vec2" },
            { "Transform",  "core::mat4x3" },
            { "Box",  "Transform_inc::Box" },
            { "Projection", "Projection_inc::Projection" },
            { "FacePositions", "std::array<float, 9>" },
            { "uint3", "core::uivec3" },
            { "uint4", "core::uivec4" },
        };
        for (auto& sd : structDescs) {
            for (auto& att : sd.attributes) {
                for (const auto& alias : hlslTocppAliases) {
                    if (att.type == alias.type) {
                        att.type = alias.member;
                        break;
                    }
                }

            }
        }
    }

    if (makefileDeps) {
        scribe->displayMakefileDeps(cout);
    } else if (makeCPlusPlus) {
        // Because there is a maximum size for literal strings declared in source we need to partition the 
        // full source string stream into pages that seems to be around that value...
        const int MAX_STRING_LITERAL = 10000;
        std::string lineToken;
        auto pageSize = lineToken.length();
        std::vector<std::shared_ptr<std::stringstream>> pages(1, std::make_shared<std::stringstream>());
        while (!destStringStream.eof()) {
            std::getline(destStringStream, lineToken);
            auto lineSize = lineToken.length() + 1;

            if (pageSize + lineSize > MAX_STRING_LITERAL) {
                pages.push_back(std::make_shared<std::stringstream>());
                // reset pageStringStream
                pageSize = 0;
            }

            (*pages.back()) << lineToken << std::endl;
            pageSize += lineSize;
        }

        std::stringstream headerStringStream;
        std::stringstream sourceStringStream;
        std::string headerFileName = destFilename + ".h";
        std::string sourceFileName = destFilename + ".cpp";

        sourceStringStream << "// File generated by Scribe " << vars["_SCRIBE_DATE"] << std::endl;

        // Write header file
        headerStringStream << "#ifndef " << targetName << "_h" << std::endl;
        headerStringStream << "#define " << targetName << "_h\n" << std::endl;
        headerStringStream << "#include <string>\n" << std::endl;
        headerStringStream << "#include <array>\n" << std::endl;
        headerStringStream << "#include <core/math/Math3D.h>\n" << std::endl;
        headerStringStream << "#include <gpu/Shader.h>\n" << std::endl;
        headerStringStream << "#include <gpu/Descriptor.h>\n" << std::endl;

        headerStringStream << "/////////    Shader Includes  /////////////" << std::endl;
        for (auto& id : includeDescs) {
            headerStringStream << "#include \"" << id.name << ".h\"" << std::endl;
        }
        headerStringStream << std::endl;

        headerStringStream << "class " << targetName << " {" << std::endl;
        headerStringStream << "public:" << std::endl;
        headerStringStream << "\tstatic const std::string& getType() { return \"" << shaderTypeString[type] << "\"; }" << std::endl;
        headerStringStream << "\tstatic const std::string& getSource() { return _source; }" << std::endl;
        headerStringStream << "\tstatic std::string getSourceFilename() { return \"" << srcFilename << "\"; }" << std::endl;

        headerStringStream << "\tstatic std::string getName() { return \"" << targetName << "\"; }" << std::endl;
        headerStringStream << "\tstatic std::string getFilename() { return \"" << targetName << targetExt << "\"; }" << std::endl;

        headerStringStream << "\ttypedef const std::string& (*SourceGetter)();" << std::endl;
        headerStringStream << "\tstatic std::pair<std::string, SourceGetter> getMapEntry() { return { getFilename(), getSource }; }" << std::endl;

        headerStringStream << "\n\t/////////    INCLUDE DESCS     /////////////" << std::endl;
        // add self as an include if an include type
        if (type == Type::INCLUDE) {
            headerStringStream << "\tstatic graphics::ShaderIncludes getSelfInclude() { return { getMapEntry() }; }" << std::endl;
        }

        headerStringStream << "\tstatic const graphics::ShaderIncludes& getShaderIncludes() {" << std::endl;
        headerStringStream << "\t\tstatic const graphics::ShaderIncludes lib = graphics::ShaderIncludes_concat(" << std::endl;
        bool first = true;
        for (auto& id : includeDescs) {
            headerStringStream << "\t\t" << (first ? "    " : "   ,") << id.name << "::getShaderIncludes()" << std::endl;
            first = false;
        }
        // add self as an include if an include type
        if (type == Type::INCLUDE) {
            headerStringStream << "\t\t" << (first ? "    " : "   ,") << "getSelfInclude()" << std::endl;
        }
        headerStringStream << "\t\t);" << std::endl;
        headerStringStream << "\t\treturn lib;" << std::endl;
        headerStringStream << "\t}" << std::endl;

        headerStringStream << "\tstatic const NameGetters& getDependentResourceNames() {" << std::endl;
        headerStringStream << "\t\tstatic const NameGetters resourceNames = {" << std::endl;
        first = true;
        for (auto& id : includeDescs) {
            headerStringStream << "\t\t" << (first ? "    &" : "   ,&") << id.name << "::getResourceNames" << std::endl;
            first = false;
        }
        headerStringStream << "\t\t};" << std::endl;
        headerStringStream << "\t\treturn resourceNames;" << std::endl;
        headerStringStream << "\t}" << std::endl;

        headerStringStream << "\n\t/////////    STRUCT DESCS     /////////////" << std::endl;
        for (auto& sd : structDescs) {
            headerStringStream << "\tstruct " << sd.name << " {" << std::endl;
            for (auto& attrib : sd.attributes) {
                headerStringStream << "\t\t" << attrib.type << " " << attrib.member << ";" << std::endl;
            }
            headerStringStream << "\t};" << std::endl;
        }

        headerStringStream << "\t/////////    RESOURCE LAYOUT     /////////////" << std::endl;

        headerStringStream << "\tstatic const graphics::DescriptorSetLayout& getPushLayout() {" << std::endl;
        headerStringStream << "\t\tstatic const graphics::DescriptorSetLayout pushLayout = {" << std::endl;
        first = true;
        for (auto& rd : pushDescs) {
            headerStringStream << "\t\t" << (first ? "    {" : "   ,{");
            headerStringStream << rd.desc << ", graphics::ShaderStage::" << shaderTypeString[type] << ", " << rd.registerslot << ", sizeof(" << rd.name << ") >> 2 }" << std::endl;
            first = false;
        }
        headerStringStream << "\t\t};" << std::endl;
        headerStringStream << "\t\treturn pushLayout;" << std::endl;
        headerStringStream << "\t}" << std::endl;

        headerStringStream << "\tstatic const std::vector<std::string>& getResourceNames() {" << std::endl;
        headerStringStream << "\t\tstatic const std::vector<std::string> bindingNames = {" << std::endl;
        first = true;
        for (auto& rd : resourceDescs) {
            headerStringStream << "\t\t" << (first ? "    \"" : "   ,\"") << rd.name << '\"' << std::endl;
            first = false;
        }
        headerStringStream << "\t\t};" << std::endl;
        headerStringStream << "\t\treturn bindingNames;" << std::endl;
        headerStringStream << "\t}" << std::endl;
        
        headerStringStream << "\tstatic const graphics::DescriptorSetLayout& getResourceLayout() {" << std::endl;
        headerStringStream << "\t\tstatic const graphics::DescriptorSetLayout bindingLayouts = {" << std::endl;
        first = true;
        for (auto& rd : resourceDescs) {
            headerStringStream << "\t\t" << (first ? "    {" : "   ,{");
            headerStringStream << rd.desc << ", graphics::ShaderStage::" << shaderTypeString[type] << ", " << rd.registerslot << ", 1 }" << std::endl;
            first = false;
        }
        headerStringStream << "\t\t};" << std::endl;
        headerStringStream << "\t\treturn bindingLayouts;" << std::endl;
        headerStringStream << "\t}" << std::endl;


        if (type != Type::INCLUDE) {
            headerStringStream << "\t/////////    SHADER INIT     /////////////" << std::endl;

            headerStringStream << "\tstatic graphics::ShaderInit getShaderInit(const std::string& entry) {" << std::endl;
            headerStringStream << "\t\tgraphics::ShaderInit shaderInit = {" << std::endl;
            headerStringStream << "\t\t\tgraphics::ShaderType::" << shaderTypeString[type] << std::endl;
            headerStringStream << "\t\t\t,entry, getSource, getSourceFilename()" << std::endl;
            headerStringStream << "\t\t\t,graphics::ShaderIncludeLib(getShaderIncludes().begin(), getShaderIncludes().end())" << std::endl;
            headerStringStream << "\t\t};" << std::endl;
            headerStringStream << "\t\treturn shaderInit;" << std::endl;
            headerStringStream << "\t}" << std::endl;
        }

        headerStringStream << "private:" << std::endl;
        headerStringStream << "\tstatic const std::string _source;" << std::endl;

        headerStringStream << "};\n" << std::endl;
        headerStringStream << "#endif // " << targetName << "_h" << std::endl;

        bool mustOutputHeader = destFilename.empty();
        // Compare with existing file
        {
            std::fstream headerFile;
            headerFile.open(headerFileName, std::fstream::in);
            if (headerFile.is_open()) {
                // Skip first line
                std::string line;
                std::stringstream previousHeaderStringStream;
                std::getline(headerFile, line);

                previousHeaderStringStream << headerFile.rdbuf();
                headerFile.close();

                mustOutputHeader = mustOutputHeader || previousHeaderStringStream.str() != headerStringStream.str();
            } else {
                mustOutputHeader = true;
            }
        }

        if (mustOutputHeader) {
            if (!destFilename.empty()) {
                // File content has changed so write it
                std::fstream headerFile;
                headerFile.open(headerFileName, std::fstream::out);
                if (headerFile.is_open()) {
                    // First line contains the date of modification
                    headerFile << sourceStringStream.str();
                    headerFile << headerStringStream.str();
                    headerFile.close();
                } else {
                    cerr << "Scribe output file <" << headerFileName << "> failed to open." << endl;
                    return 1;
                }
            } else {
                cerr << sourceStringStream.str();
                cerr << headerStringStream.str();
            }
        }

        // Write source file
        sourceStringStream << "#include \"" << targetName << ".h\"\n" << std::endl;

        sourceStringStream << "const std::string " << targetName << "::_source = std::string()";
        // Write the pages content
        for (auto page : pages) {
            sourceStringStream << "+ std::string(R\"SCRIBE(\n" << page->str() << "\n)SCRIBE\")\n";
        }
        sourceStringStream << ";\n" << std::endl << std::endl;

        // Destination stream
        if (!destFilename.empty()) {
            std::fstream sourceFile;
            sourceFile.open(sourceFileName, std::fstream::out);
            if (!sourceFile.is_open()) {
                cerr << "Scribe output file <" << sourceFileName << "> failed to open." << endl;
                return 1;
            }
            sourceFile << sourceStringStream.str();
            sourceFile.close();
        } else {
            cerr << sourceStringStream.str();
        }
    } else {
        // Destination stream
        if (!destFilename.empty()) {
            std::fstream destFileStream;
            destFileStream.open(destFilename, std::fstream::out);
            if (!destFileStream.is_open()) {
                cerr << "Scribe output file <" << destFilename << "> failed to open." << endl;
                return 1;
            }

            destFileStream << destStringStream.str();
            destFileStream.close();
        } else {
            cerr << destStringStream.str();
        }
    }

    return 0;

}

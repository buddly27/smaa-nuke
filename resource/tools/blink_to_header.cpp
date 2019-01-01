/**
 * Copyright (C) 2019, Jeremy Retailleau
 */

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>


void sanitize(
    std::string& subject, const std::string& search, const std::string& replace
)
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
}

int main(int argc, char** args)
{
    if (argc != 3) {
        std::cerr << "blink_to_header: syntax error" << std::endl;
        std::cerr << "usage: blink_to_header foo.blk foo.h" << std::endl;
        exit(0x0);
    }

    // Extract arguments.
    std::string input_file(args[1]);
    std::string output_file(args[2]);

    // Extract file name without extension.
    std::string name = input_file.substr(input_file.find_last_of("/\\") + 1);
    name = name.substr(0, name.find_last_of('.'));

    // Compute conditional variable.
    std::string var = name + "_h";
    std::transform(var.begin(), var.end(), var.begin(), ::toupper);

    // Compute output.
    std::string output;
    output += "#ifndef " + var + "\n";
    output += "#define " + var + "\n\n";
    output += "static const char* const " + name + " = \\";

    std::ifstream input_stream(input_file);
    if (input_stream.is_open())
    {
        std::string line;
        while ( getline (input_stream, line) )
        {
            sanitize(line, "\"", "\\\"");
            output += "\n\"" + line + "\\n\"";
        }
        output += ";\n\n";
        input_stream.close();
    }
    else {
        std::cerr << "Impossible to read input file." << std::endl;
        exit(0x0);
    }

    output += "#endif // " + var;

    // Extract header file.
    std::ofstream output_stream(output_file);
    if (output_stream.is_open())
    {
        output_stream << output;
        output_stream.close();
    }
    else {
        std::cerr << "Impossible to write output file." << std::endl;
        exit(0x0);
    }

    return 0;
}

#include <cstdio>
#include <string>
#include <unordered_set>
#include <vector>

#include "blifparse.hpp"

#include "blif_common.hpp"
#include "blif_lexer.hpp"
#include "blif_error.hpp"


namespace blifparse {

//.conn [Extended BLIF]
void Callback::conn(std::string /*src*/, std::string /*dst*/) {
    parse_error(-1, ".conn", "Unsupported BLIF extension");
}

//.cname [Extended BLIF]
void Callback::cname(std::string /*cell_name*/) {
    parse_error(-1, ".cname", "Unsupported BLIF extension");
}

//.attr [Extended BLIF]
void Callback::attr(std::string /*name*/, std::string /*value*/) {
    parse_error(-1, ".attr", "Unsupported BLIF extension");
}

//.param [Extended BLIF]
void Callback::param(std::string /*name*/, std::string /*value*/) {
    parse_error(-1, ".param", "Unsupported BLIF extension");
}

/*
 * Given a filename parses the file as an BLIF file
 * and returns a pointer to a struct containing all
 * the blif commands. Also follows subfile references
 * in a depth-first order.
 * See blif.h for data structure details.
 */
void blif_parse_filename(std::string filename, Callback& callback) {
    std::vector<std::string> pending_files = {std::move(filename)};
    std::unordered_set<std::string> processed_files = {};

    // According to the specification if we encounter a .search, we should start
    // parsing the subfile immediatly, and only continue parsing the current file afterwards.
    // However, this function finishes the current file first and then parses subfiles.
    // This is cleaner, as it avoids mixing callback function calls from different files.
    while(!pending_files.empty()) {
        std::string current_filename = pending_files.back();
        pending_files.pop_back();

        // if result.second == false, then the file has been processed before, so we skip it
        // if result.second == true, then the file has *not* been processed before and 
        //                           result.first is a reference to the filename
        auto result = processed_files.insert(std::move(current_filename));
        if(!result.second) {
            continue;
        }

        FILE* infile = std::fopen(result.first->c_str(), "r");
        if(infile != NULL) {
            //Parse the file
            blif_parse_file(infile, callback, pending_files, result.first->c_str());
    
            std::fclose(infile);
        } else {
            blif_error_wrap(callback, 0, "", "Could not open file '%s'.\n", result.first->c_str());
        }
    } 
}

void blif_parse_filename(const char* filename, Callback& callback) {
    blif_parse_filename(std::string(filename), callback);
}

void blif_parse_file(FILE* blif_file, Callback& callback, std::vector<std::string>& file_stack, const char* filename) {

    //Initialize the lexer
    Lexer lexer(blif_file, callback);

    //Setup the parser + lexer
    Parser parser(lexer, callback, file_stack);

    //Tell the caller the file name
    callback.filename(filename); 

    //Just before parsing starts
    callback.start_parse();

    //Do the actual parse
    int error = parser.parse();
    if(error) {
        blif_error_wrap(callback, 0, "", "File failed to parse.\n");
    }

    //Finished parsing
    callback.finish_parse();
}

}

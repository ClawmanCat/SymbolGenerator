#include <SymbolGenerator/argument_parser.hpp>
#include <SymbolGenerator/translation_unit_processor.hpp>
#include <SymbolGenerator/utility.hpp>
#include <SymbolGenerator/logger.hpp>

#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <fstream>

using std::chrono::steady_clock;


int main(int argc, char** argv) {
    steady_clock::time_point start = steady_clock::now();


    // Parse command line arguments.
    // Don't keep executable path as an argument.
    std::vector<std::string> args { argv + 1, argv + argc };

    auto& arg_parser = symgen::argument_parser::instance();
    auto& logger     = symgen::logger::instance();

    arg_parser.add_arguments(args);
    arg_parser.template require_argument<std::string>("lib");
    arg_parser.template require_argument<std::string>("i");
    arg_parser.template require_argument<std::string>("o");

    if (arg_parser.has_argument("verbose")) logger.set_level(symgen::logger::VERBOSE);


    std::string filter_args;
    for (const auto& filter_arg : { "y", "n", "yo", "no" }) {
        if (auto arg = arg_parser.get_argument<std::string>(filter_arg); arg) {
            filter_args += filter_arg;
            filter_args += " = ";
            filter_args += *arg;
            filter_args += " ";
        }
    }

    logger.normal("Symbols will be filtered according to the following settings: ", filter_args);


    // Set up output stream.
    std::ofstream stream { *arg_parser.template get_argument<std::string>("o") };
    stream << "LIBRARY " << *arg_parser.template get_argument<std::string>("lib") << "\n";
    stream << "EXPORTS\n";

    std::size_t next_index = 0;


    // Parse object files for symbols.
    std::size_t max_concurrency = arg_parser.template get_argument<long long>("j").value_or(std::thread::hardware_concurrency());
    auto object_paths = symgen::find_all_of_type(*arg_parser.template get_argument<std::string>("i"), ".obj");


    while (!object_paths.empty()) {
        std::size_t count = std::min(max_concurrency, object_paths.size());

        std::vector<std::thread> threads;
        threads.reserve(count);
        std::vector<symgen::translation_unit_processor> processors;
        processors.reserve(count);


        for (std::size_t i = 0; i < count; ++i) {
            auto& processor = processors.emplace_back();

            threads.emplace_back([path = std::move(object_paths[i]), &processor] () {
                auto name = path.filename().replace_extension().string();
                auto dir  = symgen::fs::path { path }.remove_filename();

                processor.process(dir, name);
            });
        }

        object_paths.erase(object_paths.begin(), object_paths.begin() + count);


        for (auto& thread : threads) thread.join();

        for (const auto& unit : processors) {
            for (const auto& symbol : unit.get_included_symbols()) {
                stream << symbol << " @" << next_index++ << "\n";
            }
        }

        stream << std::flush;
    }


    steady_clock::time_point stop = steady_clock::now();
    logger.verbose("Processing took ", std::chrono::duration_cast<std::chrono::milliseconds>(stop - start));

    logger.normal(
        "Generated ", *arg_parser.template get_argument<std::string>("o"),
        " with ", next_index, " symbols."
    );
}
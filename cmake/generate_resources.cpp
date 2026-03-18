/*
   File     : generate_resources
   Author   : Jefri Sibarani
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <exception>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

struct FileInfo
{
   std::string name;
   std::string path;
   size_t      size;
   std::string arraySizeVarName;
};
using FileInfoPtr  = std::shared_ptr<FileInfo>;
using FileInfoList = std::vector<FileInfoPtr>;


bool endsWidth(const std::string& input, const std::string& other)
{
#if __cpp_lib_ends_with >= 201711L
   return input.ends_with(other);
#else
   return input.size() >= other.size() && 0 == input.compare( input.size() - other.size(), other.size(), other);
#endif
}

// Convert Windows path separators to Unix path separators
std::string unixPath(const std::string& path) 
{
   std::string result = path;
   std::replace(result.begin(), result.end(), '\\', '/');
   return result;
}

// Branchless conversion of a nibble to its hexadecimal character
inline char nibbleToHex(unsigned char nib)
{
   return static_cast<char>('0' + nib + ((nib > 9) * 39)); // branchless
}

// Function to write a file 
void writeFile(const std::string& outputFileName, const std::string& outputContent) 
{
   std::ofstream outputFile(outputFileName, std::ios::binary);
   if (!outputFile.is_open()) {
      std::cerr << "Error opening file for writing: " << outputFileName << std::endl;
      return;
   }

   outputFile << outputContent;
   outputFile.close();
}

// Convert file to C array
void fileToCArray(const std::string& inputFile, 
   const std::string& relativePath, 
   FileInfoList& storage,
   std::ofstream& output) 
{
   static int counter = 0;

   std::ifstream in(inputFile, std::ios::binary);
   if (!in)
      throw std::runtime_error("Failed to open input file");

   auto inputSize = fs::file_size(inputFile);

   ++counter;

   const std::string arrayName = "resources_" + std::to_string(counter);
   const std::string arraySizeVarName = arrayName + "_size";
   
   output << "const size_t " + arraySizeVarName + " = " + std::to_string(inputSize) + ";\n";
   output << "static const unsigned char " + arrayName + "[] = {\n";

   auto info  = std::make_shared<FileInfo>();
   info->name = arrayName;
   info->path = unixPath( relativePath ); // Store with Unix path separators;
   info->size = inputSize;
   info->arraySizeVarName = arraySizeVarName;

   storage.push_back(info);

   constexpr size_t BUF_SIZE = 1 << 16; // 64 KB
   unsigned char buffer[BUF_SIZE];
   size_t col = 0;

   while (in) 
   {
      in.read(reinterpret_cast<char*>(buffer), BUF_SIZE);
      std::streamsize n = in.gcount();

      for (std::streamsize i = 0; i < n; ++i) 
      {
         //if (col % 16 == 0) {
         //   output << "\t"; // prepend tab at start of line
         //}

         unsigned char c = buffer[i];
         output << "0x" << nibbleToHex(c >> 4) << nibbleToHex(c & 0x0F);

         if (i + 1 < n || !in.eof()) 
             output << ",";
         if (++col % 16 == 0) 
             output << "\n"; // wrap line
         else 
            output << "";
      }
   }
   
   output << "\n};\n\n";
   output << "static constexpr nonstd::span<const unsigned char> " << arrayName 
          << "_span{" << arrayName << ", " << arraySizeVarName << "};\n\n";

   in.close();
}

// Read directory recursively
void readDirectory(const std::string& sourceFolder, 
   const std::string& baseFolder, 
   const std::string& outputFileName, 
   FileInfoList& storage,
   std::ofstream& output) 
{
   try
   {
      fs::path pathObj(sourceFolder);
      std::string pathObjStr =  pathObj.string();
      std::replace( pathObjStr.begin(), pathObjStr.end(), '\\', '/' );
      pathObjStr += "/";

      for (const auto& entry : fs::recursive_directory_iterator(sourceFolder)) 
      {
         if ( fs::is_regular_file(entry.path()) ) 
         {  
            std::string entryPathStr = entry.path().string();
            if (endsWidth(entryPathStr, "LICENSE.txt"))
            {
               std::cout << "Skipping file: " << entryPathStr << std::endl;
               continue;
            }
            else
               std::cout << "Reading file: " << entryPathStr << std::endl;
            
            std::replace( entryPathStr.begin(), entryPathStr.end(), '\\', '/' );

            std::string relativePath = entryPathStr.substr(pathObjStr.length());

            relativePath = baseFolder + "/" + relativePath;
            fileToCArray(entry.path().string(), relativePath, storage, output);
         }
      }
   }
   catch(const std::exception& ex)
   {
      std::cerr << "Error: " << ex.what() << std::endl;
   }
}


void generateHeaderFile(const std::string& outputFileName, const std::string& functionName)
{
   std::string output = "#pragma once\n\n";
   output += "#include <string>\n";
   output += "#include <tobasa/nonstd_span.hpp>\n\n";
   output += "namespace tbs {\n";
   output += "namespace res {\n\n";
   output += "nonstd::span<const unsigned char> " +  functionName + "(const std::string& name);\n\n";
   output += "} // namespace res\n";
   output += "} // namespace tbs";

   fs::path pathObj(outputFileName);
   std::string fileBasename =  pathObj.stem().string();
   std::string outputDirectory = pathObj.parent_path().string();
   if (outputDirectory.empty())
      outputDirectory = ".";

   std::string finalOutputFile = outputDirectory + "/" + fileBasename + ".h";
   writeFile(finalOutputFile, output);
   std::cout << "Output file generated: " << finalOutputFile << std::endl;
}


int main(int argc, char* argv[]) 
{
   if (argc != 6) 
   {
      std::cerr <<  std::endl;
      std::cerr << "Usage: " << argv[0] << " source base output function namespace" << std::endl << std::endl;

      std::cerr << " source    : source folder to read and convert the content" << std::endl;
      std::cerr << " base      : base folder name to put in the c++ output file" << std::endl;
      std::cerr << " output    : c++ output file" << std::endl;
      std::cerr << " function  : function name to put in the c++ output file" << std::endl;
      std::cerr << " namespace : namespace name to put in c++ output file" << std::endl << std::endl;
      std::cerr << "Example: " << std::endl << std::endl;
      std::cerr << "   generate_resources c:\\tmp\\src\\views views c:\\tmp\\src\\resources.cpp getTemplateResources appview" << std::endl;
      return 1;
   }

   const std::string sourceFolder   = argv[1]; // source folder to read and convert the content
   const std::string baseFolder     = argv[2]; // base folder name to put in the c++ output file
   const std::string outputFileName = argv[3]; // c++ output file
   const std::string functionName   = argv[4]; // function name to put in the c++ output file
   const std::string namespaceName  = argv[5]; // namespace name to put in c++ output file

   fs::path pathObj(outputFileName);
   std::string fileBasename =  pathObj.stem().string();
   std::string outputDirectory = pathObj.parent_path().string();

   if (!fs::exists(outputDirectory)) {
      fs::create_directories(outputDirectory);
   }   

   // Array to store converted file information
   FileInfoList storage;
   std::ofstream output(outputFileName);
   if (!output)
      throw std::runtime_error("Failed to open output file");


   output << "// disable MSVC warning\n";
   output << "#pragma warning(disable : 4838)\n";
   output << "#pragma warning(disable : 4309)\n\n";

   output << "#include <cstddef>\n";  // for size_t
   //output << "#include <tobasa/nonstd_span.hpp>\n\n";   // for std::span   
   output << "#include \"resources/" + fileBasename + ".h\"\n\n";
   output << "namespace tbs {\n";
   output << "namespace res {\n";
   output << "namespace " + namespaceName + " {\n\n";

   std::cout << "Reading directory: " << sourceFolder << std::endl;
   readDirectory(sourceFolder, baseFolder, fileBasename, storage, output);

   output << "} // namespace " + namespaceName + "\n\n";
   output << "nonstd::span<const unsigned char> " + functionName + "(const std::string& name)\n";
   output << "{\n";
   output << "   using namespace " + namespaceName + ";\n\n";

   for (const auto& meta : storage) 
   {
      output << "   if (name == \"" + meta->path + "\")\n";
      //output << "      return std::string(" + namespaceName + "::" + meta->name + ", " + meta->arraySizeVarName + ");\n";
      output << "      return nonstd::span<const unsigned char>(" +  meta->name + ", " + meta->arraySizeVarName + ");\n";
   }

   output << "\n   return {};\n";
   output << "}\n\n";
   output << "} // namespace res\n";
   output << "} // namespace tbs\n\n";
   output << "#pragma warning(default : 4838)\n";
   output << "#pragma warning(default : 4309)\n";

   output << "// re-enable MSVC warning\n";

   output.close();

   //generateHeaderFile(outputFileName, functionName);

   std::cout << "Output file generated: " << outputFileName << std::endl;

   return 0;
}


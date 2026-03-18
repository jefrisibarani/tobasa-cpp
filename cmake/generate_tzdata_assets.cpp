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

// Convert Windows path separators to Unix path separators
std::string unixPath(const std::string& path) 
{
   std::string result = path;
   std::replace(result.begin(), result.end(), '\\', '/');
   return result;
}

// Convert byte to hexadecimal string
std::string toHexa(unsigned char c)
{
   static constexpr char hex[] = "0123456789abcdef";
   std::string s(2, '0');
   s[0] = hex[c >> 4];   // high nibble
   s[1] = hex[c & 0x0F]; // low nibble
   return s;
}

// Branchless conversion of a nibble to its hexadecimal character
inline char nibbleToHex(unsigned char nib)
{
   return static_cast<char>('0' + nib + ((nib > 9) * 39)); // branchless
}

// Function to write a file 
void writeFile(const std::string& outputFileName, const std::string& outputContent) 
{
   // open iwth ios::binary, we don't want automatic CRLF translation
   std::ofstream outputFile(outputFileName, std::ios::binary); 
   if (!outputFile.is_open()) {
      std::cerr << "Error opening file for writing: " << outputFileName << std::endl;
      return;
   }

   outputFile << outputContent;
   outputFile.close();
}

/// Convert a tzdata or XML file to C array, stripping comment lines.
/// - Lines starting with '#' are skipped.
/// - XML comments <!-- ... --> are removed (even if multiline).
void fileToCArray(const std::string& inputFile,
               const std::string& relativePath,
               FileInfoList& storage,
               std::ofstream& output)
{
   static int counter = 0;

   // open in text mode, we want to clear all line endings CR and LF
   std::ifstream in(inputFile); 
   if (!in)
      throw std::runtime_error("Failed to open input file: " + inputFile);

   ++counter;

   const std::string arrayName = "resources_" + std::to_string(counter);
   const std::string arraySizeVarName = arrayName + "_size";

   // Detect XML file
   bool isXmlFile =
#if __cpp_lib_ends_with >= 201711L
      inputFile.ends_with(".xml");
#else
      inputFile.size() >= 4 && inputFile.compare(inputFile.size() - 4, 4, ".xml") == 0;
#endif

   std::ostringstream filtered;
   std::string line;
   bool insideXmlComment = false;

   while (std::getline(in, line))
   {
      // Handle IANA tzdata text files
      if (!isXmlFile)
      {
         // Skip if line starts with '#'
         if (!line.empty() && line[0] == '#')
            continue;
         else if (line.empty())
            continue;
         
         // if (!line.empty() && line.back() == '\r') // remove \r
         //    line.pop_back();

         filtered << line << "\n";
         continue;
      }
      else
      {
         // Handle XML files
         std::string outputLine;
         size_t pos = 0;
         while (pos < line.size())
         {
            if (insideXmlComment)
            {
               size_t end = line.find("-->", pos);
               if (end != std::string::npos)
               {
                  insideXmlComment = false;
                  pos = end + 3;
               }
               else {
                  pos = line.size(); // Still inside comment, skip whole line
               }
            }
            else
            {
               size_t start = line.find("<!--", pos);
               if (start != std::string::npos)
               {
                  // Append content before comment start
                  outputLine.append(line, pos, start - pos);

                  size_t end = line.find("-->", start + 4);
                  if (end != std::string::npos)
                  {
                     // Comment ends on same line — continue parsing after it
                     pos = end + 3;
                  }
                  else
                  {
                     // Comment continues to next line
                     insideXmlComment = true;
                     pos = line.size();
                  }
               }
               else
               {
                  // No comment found, keep full rest of line
                  outputLine.append(line, pos, std::string::npos);
                  break;
               }
            }
         }

         // if (!outputLine.empty() && outputLine.back() == '\r')
         //    outputLine.pop_back();

         // If the line was entirely inside a comment, outputLine will be empty — skip it
         if (!outputLine.empty() || !insideXmlComment)
            filtered << outputLine << "\n";
      }
   }

   std::string filteredData = filtered.str();
   size_t inputSize = filteredData.size();

   // Register file info
   auto info  = std::make_shared<FileInfo>();
   info->name = arrayName;
   info->path = unixPath(relativePath);
   info->size = inputSize;
   info->arraySizeVarName = arraySizeVarName;
   storage.push_back(info);

   {
      if (!fs::exists("tzdata_filtered")) {
         fs::create_directories("tzdata_filtered");
      }
      // output also the 'filtered text' version
      // write filtered text version to a file in the current directory
      std::string filename = fs::path(inputFile).filename().string();
      std::string textFilename = "tzdata_filtered/" + filename;
      std::string textContent = std::string("// original path: ") + info->path + "\n" + filteredData;
      writeFile(textFilename, textContent);
      //std::cout << "Filtered text written to: " << textFilename << std::endl;
   }


   // Emit as C array
   output << "const size_t " << arraySizeVarName << " = " << inputSize << ";\n";
   output << "static const unsigned char " << arrayName << "[] = {\n";

   size_t col = 0;
   for (unsigned char c : filteredData)
   {
      output << "0x" << nibbleToHex(c >> 4) << nibbleToHex(c & 0x0F) << ",";
      if (++col % 16 == 0)
         output << "\n";
   }

   output << "\n};\n\n";
   output << "static constexpr nonstd::span<const unsigned char> "
         << arrayName << "_span{" << arrayName << ", " << arraySizeVarName << "};\n\n";

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
         if (fs::is_regular_file(entry.path())) 
         {
            std::string entryPathStr = entry.path().string();
            std::cout << "Reading file: " << entryPathStr << std::endl;
            
            std::replace( entryPathStr.begin(), entryPathStr.end(), '\\', '/' );
            std::string relativePath = entryPathStr.substr(pathObjStr.length());
            relativePath = baseFolder + "/" + relativePath;
            fileToCArray(entryPathStr, relativePath, storage, output);
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
   output += "#include <tobasa/span.h>\n\n";
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
      std::cerr << "   generate_tzdata_assets_file c:\\tmp\\src\\tzdata tzdata c:\\tmp\\src\\tzdata_assets.cpp getTzdataAsset tzdata" << std::endl;
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

   generateHeaderFile(outputFileName, functionName);

   std::cout << "Output file generated: " << outputFileName << std::endl;

   return 0;
}
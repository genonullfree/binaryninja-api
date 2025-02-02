// This is just bin-info but with an enterprise license checkout

#include <sys/stat.h>

#include <iostream>
#include <cstdlib>

#include "binaryninjacore.h"
#include "binaryninjaapi.h"
#include "enterprise.h"

using namespace BinaryNinja;
using namespace std;

bool is_file(char* fname)
{
	struct stat buf;
	if (stat(fname, &buf) == 0 && (buf.st_mode & S_IFREG) == S_IFREG)
		return true;

	return false;
}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		cerr << "USAGE: " << argv[0] << " <file_name>" << endl;
		exit(-1);
	}

	char* fname = argv[1];
	if (!is_file(fname))
	{
		cerr << "Error: " << fname << " is not a regular file" << endl;
		exit(-1);
	}

	EnterpriseServer::LicenseCheckout _{};

	/* In order to initiate the bundled plugins properly, the location
	 * of where bundled plugins directory is must be set. */
	SetBundledPluginDirectory(GetBundledPluginDirectory());
	InitPlugins();

	Ref<BinaryData> bd = new BinaryData(new FileMetadata(), argv[1]);
	Ref<BinaryView> bv;
	for (auto type : BinaryViewType::GetViewTypes())
	{
		if (type->IsTypeValidForData(bd) && type->GetName() != "Raw")
		{
			bv = type->Create(bd);
			break;
		}
	}

	if (!bv || bv->GetTypeName() == "Raw")
	{
		fprintf(stderr, "Input file does not appear to be an exectuable\n");
		return -1;
	}

	bv->UpdateAnalysisAndWait();

	cout << "Target:   " << fname << endl << endl;
	cout << "TYPE:     " << bv->GetTypeName() << endl;
	cout << "START:    0x" << hex << bv->GetStart() << endl;
	cout << "ENTRY:    0x" << hex << bv->GetEntryPoint() << endl;
	cout << "PLATFORM: " << bv->GetDefaultPlatform()->GetName() << endl;
	cout << endl;

	cout << "---------- 10 Functions ----------" << endl;
	int x = 0;
	for (auto func : bv->GetAnalysisFunctionList())
	{
		cout << hex << func->GetStart() << " " << func->GetSymbol()->GetFullName() << endl;
		if (++x >= 10)
			break;
	}
	cout << endl;

	cout << "---------- 10 Strings ----------" << endl;
	x = 0;
	for (auto str_ref : bv->GetStrings())
	{
		char* str = (char*)malloc(str_ref.length + 1);
		bv->Read(str, str_ref.start, str_ref.length);
		str[str_ref.length] = 0;

		cout << hex << str_ref.start << " (" << dec << str_ref.length << ") " << str << endl;
		free(str);

		if (++x >= 10)
			break;
	}

	// Shutting down is required to allow for clean exit of the core
	BNShutdown();

	return 0;
}

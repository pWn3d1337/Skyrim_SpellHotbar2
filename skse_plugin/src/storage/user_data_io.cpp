#include "user_data_io.h"

#include "Shlobj.h"


namespace SpellHotbar::Storage::IO {

    constexpr std::string_view user_dir{ "My Games/Skyrim Special Edition/SpellHotbar/" };

	std::filesystem::path IO::get_user_dir()
	{
        std::filesystem::path ret;

        TCHAR mydoc_path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPath(NULL,
            CSIDL_MYDOCUMENTS,
            NULL,
            SHGFP_TYPE_CURRENT,
            mydoc_path)))
        {
            ret = std::filesystem::path(mydoc_path) / user_dir;
        }

		return ret;
	}

}
#include "DOA6/G1mFile.h"
#include "debug.h"

std::string program_dir;

static bool text_oid_exists(const std::string &path)
{
    if (!Utils::FileExists(path))
        return false;

    size_t size;
    uint8_t *buf;

    buf = Utils::ReadFile(path, &size, false);
    if (!buf)
        return false;

    bool ret = true;

    for (size_t i = 0; i < size; i++)
    {
        if (buf[i] == 0)
        {
            ret = false;
            break;
        }
    }

    delete[] buf;
    return ret;
}

static void load_bone_names(const std::string &path, G1mFile &g1m)
{
    std::string oid_file = path.substr(0, path.length()-3) + "oid";
    std::string costume_oid;
    std::string hair_oid;
    std::string face_oid;

    int type = -1; // 0 = costume, 1 = hair, 2 = face

    if (Utils::FileExists("costume.oid"))
    {
        costume_oid = "costume.oid";
    }
    else
    {
        std::string path = Utils::MakePathString(program_dir, "costume.oid");

        if (Utils::FileExists(path))
        {
            costume_oid = path;
        }
    }

    if (Utils::FileExists("hair.oid"))
    {
        hair_oid = "hair.oid";
    }
    else
    {
        std::string path = Utils::MakePathString(program_dir, "hair.oid");

        if (Utils::FileExists(path))
        {
            hair_oid = path;
        }
    }

    if (Utils::FileExists("face.oid"))
    {
        face_oid = "face.oid";
    }
    else
    {
        std::string path = Utils::MakePathString(program_dir, "face.oid");

        if (Utils::FileExists(path))
        {
            face_oid = path;
        }
    }

    std::string fn = Utils::GetFileNameString(path);

    if (fn.find("_HAIR_") != std::string::npos)
    {
        type = 1;
    }
    else if (fn.find("_FACE_") != std::string::npos)
    {
        type = 2;
    }
    else if ((g1m.GetNumBonesID() >= 800 && g1m.GetNumBonesID() <= 900) ||
             (fn.find("_COS_") != std::string::npos && fn.find("OUTGAME") == std::string::npos && fn.find("CHRSEL") == std::string::npos))
    {
        type = 0;
    }

    if (!text_oid_exists(oid_file) || !g1m.LoadBoneNames(oid_file))
    {
        g1m.SetDefaultBoneNames();

        if (type == 0)
        {
            if (costume_oid.length() == 0 || !g1m.LoadBoneNames(costume_oid))
            {
                g1m.SetDefaultBoneNames();
            }
        }

        else if (type == 1)
        {
            if (hair_oid.length() == 0 || !g1m.LoadBoneNames(hair_oid))
            {
                g1m.SetDefaultBoneNames();
            }
        }

        else if (type == 2)
        {
            if (face_oid.length() == 0 || !g1m.LoadBoneNames(face_oid))
            {
                g1m.SetDefaultBoneNames();
            }
        }
    }
}

static std::string get_path_from_input()
{
	static char str[MAX_PATH];
	gets(str);
	
	std::string path = str;
	Utils::TrimString(path);
	
	if (path.length() > 0)
	{
		if (path[0] == '"')
			path = path.substr(1);
		
		if (path.length() > 0 && path.back() == '"')
			path.pop_back();
	}
	
	return path;
}

bool import_g1m_data(const std::string &path)
{
	G1mFile g1m;
	
	if (!Utils::EndsWith(path, ".g1m", false))
	{
		DPRINTF("Error: File should have .g1m extension.\n");
		return false;
	}
	
	g1m.SetParseNun(false);
	
	if (!g1m.LoadFromFile(path.c_str()))
		return false;
	
	load_bone_names(path, g1m);
	
	std::string dir_path = Utils::GetFileNameString(path);
	dir_path = dir_path.substr(0, dir_path.length()-4);
	
	if (!Utils::DirExists(dir_path))
	{
		DPRINTF("Now enter a directory path with the vb/ib files (or drag and drop the directory) and press enter.\n");
		
		dir_path = get_path_from_input();
		
		if (!Utils::DirExists(dir_path))
		{
			DPRINTF("That directory doesn't exist!\n");
			return false;
		}
	}
	
	size_t num_submeshes = g1m.GetNumSubmeshes();
	size_t num_imported = 0;
	
	for (size_t i = 0; i < num_submeshes; i++)
	{
		std::string vb_path = Utils::MakePathString(dir_path, Utils::ToString(i) + ".vb");
		std::string ib_path = Utils::MakePathString(dir_path, Utils::ToString(i) + ".ib");
		std::string vgmap_path = Utils::MakePathString(dir_path, Utils::ToString(i) + ".vgmap");
		
		if (Utils::FileExists(vb_path) && Utils::FileExists(ib_path))
		{
			DPRINTF("Importing submesh %Id...\n", i);
			
			if (!Utils::FileExists(vgmap_path))
				vgmap_path.clear();
			
			if (!g1m.ImportSubmeshFrom3DM(i, vb_path, ib_path, vgmap_path))
			{
				DPRINTF("Import of submesh %Id failed!\n", i);
				return false;
			}
			
			num_imported++;
		}
	}
	
	if (num_imported == 0)
	{
		DPRINTF("There weren't any files to import. Make sure to name the .vb/.ib files like 0.vb, 0.ib, 1.vb, 1.ib\n");
		return false;
	}
	else
	{
		if (!g1m.SaveToFile(path))
			return false;
		
		UPRINTF("%Id submeshes were imported.\n", num_imported);
	}
	
	UPRINTF("Success!\n");
	return true;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        DPRINTF("Bad usage. Usage: %s file\n", argv[0]);
		UPRINTF("Press enter to exit.");
		getchar();
        return -1;
    }
	
	if (!strchr(argv[0], '\\') && !strchr(argv[0], '/'))
    {
        program_dir = "./";
    }
    else
    {
        program_dir = Utils::GetDirNameString(argv[0]);
    }

    int ret = import_g1m_data(Utils::NormalizePath(argv[1]));

	fseek(stdin, 0, SEEK_END);
	UPRINTF("Press enter to exit.");
    getchar();

    return ret;
}

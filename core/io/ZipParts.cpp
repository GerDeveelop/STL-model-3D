#include "ZipParts.h"
#include "StlReader.h"
#include <miniz.h>
#include <algorithm>
#include <set>
#include <stdexcept>

namespace stlcut {
std::map<std::string,std::vector<unsigned char>> readAppearanceParts(std::span<const unsigned char> data,std::stop_token stop) {
    if(stop.stop_requested())throw ImportCancelled{};
    if(data.size()>maxFileBytes)throw std::runtime_error("3MF supera 256 MiB.");
    mz_zip_archive zip{};
    if(!mz_zip_reader_init_mem(&zip,data.data(),data.size(),0))throw std::runtime_error("ZIP 3MF invalido para apariencia.");
    struct Close {mz_zip_archive* zip;~Close(){mz_zip_reader_end(zip);}} close{&zip};
    const auto count=mz_zip_reader_get_num_files(&zip);
    if(count>4096)throw std::runtime_error("ZIP 3MF supera 4096 entradas.");
    std::uint64_t total=0;std::set<std::string> names;
    std::map<std::string,std::vector<unsigned char>> parts;
    for(mz_uint i=0;i<count;++i) {
        if(stop.stop_requested())throw ImportCancelled{};
        mz_zip_archive_file_stat stat{};
        if(!mz_zip_reader_file_stat(&zip,i,&stat)||stat.m_uncomp_size>maxFileBytes-total||
           stat.m_is_encrypted||(stat.m_method!=0&&stat.m_method!=8))throw std::runtime_error("Entrada ZIP fuera de limites o formato admitido.");
        total+=stat.m_uncomp_size;
        const auto length=mz_zip_reader_get_filename(&zip,i,nullptr,0);
        if(!length||length>1025)throw std::runtime_error("Nombre de entrada ZIP no admitido.");
        std::string name(length,'\0');mz_zip_reader_get_filename(&zip,i,name.data(),length);name.resize(length-1);
        if(name.empty()||name.find('\0')!=std::string::npos||name.front()=='/'||name.find('\\')!=std::string::npos||name.find(':')!=std::string::npos||
           name==".."||name.starts_with("../")||name.find("/../")!=std::string::npos||name.ends_with("/..")||
           name.starts_with("./")||name.find("/./")!=std::string::npos||!names.insert(name).second)
            throw std::runtime_error("Ruta ZIP ambigua o duplicada; no se extrae al disco.");
        if(stat.m_is_directory)continue;
        const bool selected=name.ends_with(".model")||name=="Metadata/model_settings.config"||name=="Metadata/project_settings.config";
        if(!selected)continue;
        if(name.ends_with(".config")&&stat.m_uncomp_size>4*1024*1024)throw std::runtime_error("Metadatos del laminador superan 4 MiB por archivo.");
        struct Output {std::vector<unsigned char> bytes;size_t expected;std::stop_token stop;bool failed{};};
        Output output{{},static_cast<size_t>(stat.m_uncomp_size),stop};output.bytes.reserve(output.expected);
        const auto callback=[](void* user,mz_uint64 offset,const void* buffer,size_t size)->size_t {
            auto& out=*static_cast<Output*>(user);
            if(out.stop.stop_requested()||offset!=out.bytes.size()||size>out.expected-out.bytes.size()){out.failed=true;return 0;}
            try {const auto* p=static_cast<const unsigned char*>(buffer);out.bytes.insert(out.bytes.end(),p,p+size);}
            catch(...) {out.failed=true;return 0;}
            return size;
        };
        const bool ok=mz_zip_reader_extract_to_callback(&zip,i,callback,&output,0)!=0;
        if(stop.stop_requested())throw ImportCancelled{};
        if(!ok||output.failed||output.bytes.size()!=output.expected)throw std::runtime_error("ZIP: descompresion, CRC o longitud de apariencia invalida.");
        parts.emplace(std::move(name),std::move(output.bytes));
    }
    return parts;
}
}

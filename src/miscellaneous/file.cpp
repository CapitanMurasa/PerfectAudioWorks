#include "file.h"
#include "misc.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <vector>

#include <taglib/taglib.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/audioproperties.h>
#include <taglib/tstring.h>

#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/id3v2frame.h>
#include <taglib/attachedpictureframe.h>

#include <taglib/flacfile.h>

#include <taglib/wavfile.h>
#include <taglib/rifffile.h>

#include <taglib/opusfile.h>
#include <taglib/xiphcomment.h>

#include <taglib/mp4file.h>
#include <taglib/mp4tag.h>
#include <taglib/mp4item.h>
#include <taglib/mp4coverart.h>

#ifdef _WIN32
#define str_cmp_idx _stricmp
#else
#define str_cmp_idx strcasecmp
#endif

bool CanItUseExternalAlbumart = false;

static void copy_taglib_string(const TagLib::String& source, char* dest, size_t size) {
    if (source.isEmpty()) {
        dest[0] = '\0';
        return;
    }
    strncpy(dest, source.to8Bit(true).c_str(), size - 1);
    dest[size - 1] = '\0';
}

static void extract_cover_art(TagLib::File* file, FileInfo* info, const char* ext) {
    info->cover_image = nullptr;
    info->cover_size = 0;

    // --- MP3 ---
    if (str_cmp_idx(ext, "mp3") == 0) {
        TagLib::MPEG::File* mpegFile = dynamic_cast<TagLib::MPEG::File*>(file);
        if (mpegFile && mpegFile->ID3v2Tag()) {
            TagLib::ID3v2::FrameList l = mpegFile->ID3v2Tag()->frameListMap()["APIC"];
            if (!l.isEmpty()) {
                TagLib::ID3v2::AttachedPictureFrame* f =
                    static_cast<TagLib::ID3v2::AttachedPictureFrame*>(l.front());

                info->cover_size = f->picture().size();
                info->cover_image = (unsigned char*)malloc(info->cover_size);
                if (info->cover_image) {
                    memcpy(info->cover_image, f->picture().data(), info->cover_size);
                }
            }
        }
    }
    // --- FLAC ---
    else if (str_cmp_idx(ext, "flac") == 0) {
        TagLib::FLAC::File* flacFile = dynamic_cast<TagLib::FLAC::File*>(file);
        if (flacFile && !flacFile->pictureList().isEmpty()) {
            TagLib::FLAC::Picture* pic = flacFile->pictureList().front();

            info->cover_size = pic->data().size();
            info->cover_image = (unsigned char*)malloc(info->cover_size);
            if (info->cover_image) {
                memcpy(info->cover_image, pic->data().data(), info->cover_size);
            }
        }
    }
    // --- WAV ---
    else if (str_cmp_idx(ext, "wav") == 0) {
        TagLib::RIFF::WAV::File* wavFile = dynamic_cast<TagLib::RIFF::WAV::File*>(file);
        if (wavFile && wavFile->ID3v2Tag()) {
            TagLib::ID3v2::FrameList l = wavFile->ID3v2Tag()->frameListMap()["APIC"];
            if (!l.isEmpty()) {
                TagLib::ID3v2::AttachedPictureFrame* f =
                    static_cast<TagLib::ID3v2::AttachedPictureFrame*>(l.front());

                info->cover_size = f->picture().size();
                info->cover_image = (unsigned char*)malloc(info->cover_size);
                if (info->cover_image) {
                    memcpy(info->cover_image, f->picture().data(), info->cover_size);
                }
            }
        }
    }
    // --- OPUS ---
    else if (str_cmp_idx(ext, "opus") == 0) {
        TagLib::Ogg::Opus::File* opusFile = dynamic_cast<TagLib::Ogg::Opus::File*>(file);
        if (opusFile && opusFile->tag()) {
            TagLib::Ogg::XiphComment* xiphTag = dynamic_cast<TagLib::Ogg::XiphComment*>(opusFile->tag());
            if (xiphTag && !xiphTag->pictureList().isEmpty()) {
                TagLib::FLAC::Picture* pic = xiphTag->pictureList().front();

                info->cover_size = pic->data().size();
                info->cover_image = (unsigned char*)malloc(info->cover_size);
                if (info->cover_image) {
                    memcpy(info->cover_image, pic->data().data(), info->cover_size);
                }
            }
        }
    }
    // --- M4A / MP4 / AAC ---
    else if (str_cmp_idx(ext, "m4a") == 0 || str_cmp_idx(ext, "mp4") == 0 || str_cmp_idx(ext, "aac") == 0) {
        TagLib::MP4::File* mp4File = dynamic_cast<TagLib::MP4::File*>(file);
        if (mp4File && mp4File->tag()) {
            TagLib::MP4::Tag* mp4Tag = dynamic_cast<TagLib::MP4::Tag*>(mp4File->tag());

            if (mp4Tag && mp4Tag->itemMap().contains("covr")) {
                TagLib::MP4::Item coverItem = mp4Tag->itemMap()["covr"];
                TagLib::MP4::CoverArtList coverList = coverItem.toCoverArtList();

                if (!coverList.isEmpty()) {
                    TagLib::MP4::CoverArt cover = coverList.front();

                    info->cover_size = cover.data().size();
                    info->cover_image = (unsigned char*)malloc(info->cover_size);
                    if (info->cover_image) {
                        memcpy(info->cover_image, cover.data().data(), info->cover_size);
                    }
                }
            }
        }
    }
}

static void get_albumart_fromfolder(FileInfo* info) {
    std::vector<std::string> targets = { "folder.jpg", "cover.jpg", "album.jpg" };

    if (!info->filename && info->filename != nullptr) return;

    std::filesystem::path songPath(info->filename); 
    if (!songPath.has_parent_path()) return;

    std::filesystem::path parentDir = songPath.parent_path();

    for (const auto& name : targets) {
        std::filesystem::path fullPath = parentDir / name;
        std::error_code ec;
        if (std::filesystem::exists(fullPath, ec)) {
            std::ifstream input(fullPath, std::ios::binary | std::ios::ate);
            if (input) {
                std::streamsize size = input.tellg();
                input.seekg(0, std::ios::beg);

                if (size > 0) {
                    info->cover_size = static_cast<size_t>(size);
                    info->cover_image = (unsigned char*)malloc(info->cover_size);

                    if (info->cover_image) {
                        input.read((char*)info->cover_image, size);
                    }
                }
                return;
            }
        }
    }
}

static int process_file_internal(TagLib::FileRef& f, FileInfo* info, const char* ext) {
    if (f.isNull() || !f.file() || !f.file()->isValid()) {
        return 1;
    }

    if (ext) {
        strncpy(info->format, ext, 31);
        info->format[31] = '\0';
    }
    else {
        info->format[0] = '\0';
    }

    if (f.audioProperties()) {
        TagLib::AudioProperties* props = f.audioProperties();
        info->length = props->length();
        info->bitrate = props->bitrate();
        info->sampleRate = props->sampleRate();
        info->channels = props->channels();
    }
    else {
        info->length = 0; info->bitrate = 0; info->sampleRate = 0; info->channels = 0;
    }

    if (f.tag()) {
        TagLib::Tag* tag = f.tag();
        copy_taglib_string(tag->title(), info->title, INFO_BUFFER_SIZE);
        copy_taglib_string(tag->artist(), info->artist, INFO_BUFFER_SIZE);
        copy_taglib_string(tag->album(), info->album, INFO_BUFFER_SIZE);
        copy_taglib_string(tag->genre(), info->genre, INFO_BUFFER_SIZE);
    }
    else {
        info->title[0] = '\0'; info->artist[0] = '\0';
        info->album[0] = '\0'; info->genre[0] = '\0';
    }

    extract_cover_art(f.file(), info, ext);
    if (info->cover_image == nullptr && CanItUseExternalAlbumart == true) {
        get_albumart_fromfolder(info);
    }

    return 0;
}

const char* get_file_format(const char* filename) {
    const char* dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

#ifdef _WIN32

int get_metadata_w(const wchar_t* filename_w, FileInfo* info) {
    memset(info, 0, sizeof(FileInfo));

    info->filename = filename_w;

    std::wstring fn(filename_w);
    size_t dotPos = fn.find_last_of(L".");
    std::string ext = "";
    if (dotPos != std::wstring::npos) {
        std::wstring wExt = fn.substr(dotPos + 1);
        ext.assign(wExt.begin(), wExt.end());
    }

    TagLib::FileRef f(filename_w);
    return process_file_internal(f, info, ext.c_str());
}

#else
int get_metadata(const char* filename, FileInfo* info) {
    memset(info, 0, sizeof(FileInfo));

    info->filename = filename;

    std::string fn(filename);
    size_t dotPos = fn.find_last_of(".");
    std::string ext = (dotPos != std::string::npos) ? fn.substr(dotPos + 1) : "";

    TagLib::FileRef f(filename);
    return process_file_internal(f, info, ext.c_str());
}
#endif

void FileInfo_cleanup(FileInfo* info) {
    if (info->cover_image) {
        free(info->cover_image);
        info->cover_image = nullptr;
    }
    info->cover_size = 0;
}

void setCanUseExternalAlbumart(bool value) {
    CanItUseExternalAlbumart = value;
}
#include "file.h"
#include <stdio.h>
#include <string.h> 
#include <stdlib.h> 
#include <wchar.h>
#include <iostream>

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tstring.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/flacfile.h>

static void copy_tstring_to_char(const TagLib::String& tstr, char* dest, size_t dest_size) {
    if (tstr.isEmpty() || dest_size == 0) {
        if (dest_size > 0) dest[0] = '\0';
        return;
    }
    std::string s = tstr.to8Bit(true);
    strncpy(dest, s.c_str(), dest_size - 1);
    dest[dest_size - 1] = '\0';
}

#ifdef _WIN32
static const char* get_file_format_w(const wchar_t* filename_w) {
    FILE* file = _wfopen(filename_w, L"rb");
    if (!file) return "Unknown";

    unsigned char buffer[64];
    size_t n = fread(buffer, 1, sizeof(buffer), file);
    fclose(file);

    if (n >= 12) {
        if (memcmp(buffer, "RIFF", 4) == 0) return "WAV";
        if (memcmp(buffer, "fLaC", 4) == 0) return "FLAC";
        if (memcmp(buffer, "OggS", 4) == 0) return "OGG";
        if (memcmp(buffer, "ID3", 3) == 0) return "MP3";
        if ((buffer[0] == 0xFF) && ((buffer[1] & 0xE0) == 0xE0)) return "MP3";
        if (memcmp(buffer, "MThd", 4) == 0) return "MIDI";
        if (memcmp(buffer, "OpusHead", 8) == 0) return "OPUS";
        if (memcmp(buffer + 28, "OpusHead", 8) == 0) return "OPUS";
        if (memcmp(buffer, "\x30\x26\xB2\x75", 4) == 0) return "WMA";
        if (memcmp(buffer + 4, "ftypM4A", 7) == 0 || memcmp(buffer + 4, "ftypisom", 8) == 0) return "M4A";
    }
    return "Unknown";
}
#endif

const char* get_file_format(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) return "Unknown";

    unsigned char buffer[64];
    size_t n = fread(buffer, 1, sizeof(buffer), file);
    fclose(file);

    if (n >= 12) {
        if (memcmp(buffer, "RIFF", 4) == 0) return "WAV";
        if (memcmp(buffer, "fLaC", 4) == 0) return "FLAC";
        if (memcmp(buffer, "OggS", 4) == 0) return "OGG";
        if (memcmp(buffer, "ID3", 3) == 0) return "MP3";
        if ((buffer[0] == 0xFF) && ((buffer[1] & 0xE0) == 0xE0)) return "MP3";
        if (memcmp(buffer, "MThd", 4) == 0) return "MIDI";
        if (memcmp(buffer, "OpusHead", 8) == 0) return "OPUS";
        if (memcmp(buffer + 28, "OpusHead", 8) == 0) return "OPUS";
        if (memcmp(buffer, "\x30\x26\xB2\x75", 4) == 0) return "WMA";
        if (memcmp(buffer + 4, "ftypM4A", 7) == 0 || memcmp(buffer + 4, "ftypisom", 8) == 0) return "M4A";
    }
    return "Unknown";
}

extern "C" {

#ifdef _WIN32
    int get_metadata_w(const wchar_t* filename_w, FileInfo* info) {
        memset(info, 0, sizeof(FileInfo));

        std::cout << filename_w << std::endl;

        const char* type = get_file_format_w(filename_w);
        strncpy(info->format, type, sizeof(info->format) - 1);
        info->format[sizeof(info->format) - 1] = '\0';

        TagLib::FileRef fileRef(filename_w);

        if (fileRef.isNull() || !fileRef.tag()) {
            return 0;
        }

        TagLib::Tag* tag = fileRef.tag();

        copy_tstring_to_char(tag->title(), info->title, INFO_BUFFER_SIZE);
        copy_tstring_to_char(tag->artist(), info->artist, INFO_BUFFER_SIZE);
        copy_tstring_to_char(tag->album(), info->album, INFO_BUFFER_SIZE);
        copy_tstring_to_char(tag->genre(), info->genre, INFO_BUFFER_SIZE);

        TagLib::ByteVector imgData;

        if (strcmp(type, "MP3") == 0) {
            TagLib::MPEG::File mpegFile(filename_w);
            if (mpegFile.ID3v2Tag()) {
                TagLib::ID3v2::FrameList apicFrames = mpegFile.ID3v2Tag()->frameList("APIC");
                if (!apicFrames.isEmpty()) {
                    TagLib::ID3v2::AttachedPictureFrame* bestFrame = nullptr;
                    for (auto* frame : apicFrames) {
                        auto* picFrame = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(frame);
                        if (picFrame->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover) {
                            bestFrame = picFrame;
                            break;
                        }
                    }
                    if (!bestFrame) {
                        bestFrame = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(apicFrames.front());
                    }
                    imgData = bestFrame->picture();
                }
            }
        }
        else if (strcmp(type, "FLAC") == 0) {
            TagLib::FLAC::File flacFile(filename_w);
            auto picList = flacFile.pictureList();
            if (!picList.isEmpty()) {
                TagLib::FLAC::Picture* bestPic = nullptr;
                for (auto* pic : picList) {
                    if (pic->type() == TagLib::FLAC::Picture::FrontCover) {
                        bestPic = pic;
                        break;
                    }
                }
                if (!bestPic) {
                    bestPic = picList[0];
                }
                imgData = bestPic->data();
            }
        }

        if (!imgData.isEmpty()) {
            info->cover_size = imgData.size();
            info->cover_image = (unsigned char*)malloc(info->cover_size);
            if (info->cover_image) {
                memcpy(info->cover_image, imgData.data(), info->cover_size);
            }
            else {
                info->cover_size = 0;
            }
        }

        return 0;
    }
#endif

    int get_metadata(const char* filename, FileInfo* info) {
        memset(info, 0, sizeof(FileInfo));

        const char* type = get_file_format(filename);
        strncpy(info->format, type, sizeof(info->format) - 1);
        info->format[sizeof(info->format) - 1] = '\0';

        TagLib::FileRef fileRef(filename);

        if (fileRef.isNull() || !fileRef.tag()) {
            return 0;
        }

        TagLib::Tag* tag = fileRef.tag();

        copy_tstring_to_char(tag->title(), info->title, INFO_BUFFER_SIZE);
        copy_tstring_to_char(tag->artist(), info->artist, INFO_BUFFER_SIZE);
        copy_tstring_to_char(tag->album(), info->album, INFO_BUFFER_SIZE);
        copy_tstring_to_char(tag->genre(), info->genre, INFO_BUFFER_SIZE);

        TagLib::ByteVector imgData;

        if (strcmp(type, "MP3") == 0) {
            TagLib::MPEG::File mpegFile(filename);
            if (mpegFile.ID3v2Tag()) {
                TagLib::ID3v2::FrameList apicFrames = mpegFile.ID3v2Tag()->frameList("APIC");
                if (!apicFrames.isEmpty()) {
                    TagLib::ID3v2::AttachedPictureFrame* bestFrame = nullptr;
                    for (auto* frame : apicFrames) {
                        auto* picFrame = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(frame);
                        if (picFrame->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover) {
                            bestFrame = picFrame;
                            break;
                        }
                    }
                    if (!bestFrame) {
                        bestFrame = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(apicFrames.front());
                    }
                    imgData = bestFrame->picture();
                }
            }
        }
        else if (strcmp(type, "FLAC") == 0) {
            TagLib::FLAC::File flacFile(filename);
            auto picList = flacFile.pictureList();
            if (!picList.isEmpty()) {
                TagLib::FLAC::Picture* bestPic = nullptr;
                for (auto* pic : picList) {
                    if (pic->type() == TagLib::FLAC::Picture::FrontCover) {
                        bestPic = pic;
                        break;
                    }
                }
                if (!bestPic) {
                    bestPic = picList[0];
                }
                imgData = bestPic->data();
            }
        }

        if (!imgData.isEmpty()) {
            info->cover_size = imgData.size();
            info->cover_image = (unsigned char*)malloc(info->cover_size);
            if (info->cover_image) {
                memcpy(info->cover_image, imgData.data(), info->cover_size);
            }
            else {
                info->cover_size = 0;
            }
        }

        return 0;
    }

    void FileInfo_cleanup(FileInfo* info) {
        if (info && info->cover_image) {
            free(info->cover_image);
            info->cover_image = NULL;
            info->cover_size = 0;
        }
    }

}
#!/usr/bin/env python3

# GSF spec: https://www.caitsith2.com/gsf/gsf%20spec.txt
# PSF spec: https://web.archive.org/web/20031018001256/http://www.neillcorlett.com/psf/psf_format.txt

import zlib
import sys
import json

def parse_songname_from_tags(tag_data):
    if tag_data[0:5].decode("ascii") != "[TAG]":
        raise Exception("Illegal Tag start")
    tag_data = tag_data[5:]
    tag_strings = tag_data.decode("utf-8").splitlines()

    # now look for a tag called "title"
    for tag in tag_strings:
        if not tag.startswith("title="):
            continue
        return tag[6:]
    return "unnamed song"


def add_minigsf_to_playlist(minigsf_file, playlist):
    with open(minigsf_file, "rb") as minigsf_handle:
        # load data from minigsf
        minigsf_data = minigsf_handle.read()
        if minigsf_data[0:3].decode("ascii") != "PSF":
            raise Exception("Illegal file magic")
        reserved_data_size = (
                (minigsf_data[4]) |
                (minigsf_data[5] << 8) |
                (minigsf_data[6] << 16) |
                (minigsf_data[7] << 24))
        program_data_size = (
                (minigsf_data[8]) |
                (minigsf_data[9] << 8) |
                (minigsf_data[10] << 16) |
                (minigsf_data[11] << 24))
        # I am too lazy to implement CRC32 check at the time
        program_data = zlib.decompress(minigsf_data[0x10 + reserved_data_size : 0x10 + reserved_data_size + program_data_size])
        if len(program_data) != 14:
            raise Exception("This converter only supports GSF with 14 bytes program data")
        # get song num
        song_num = (program_data[12] | (program_data[13] << 8))
        # get song name from tags
        tag_data = minigsf_data[0x10 + reserved_data_size + program_data_size:]
        if len(tag_data) == 0:
            song_name = "unnamed song"
        else:
            song_name = parse_songname_from_tags(tag_data)
        playlist.append({ "index" : song_num, "name" : song_name })

# GSF often have the track order encoded in the file name
minigsfs = sorted(sys.argv[1:])

# remove all non minigsf files
minigsfs = [n for n in minigsfs if n.lower().endswith(".minigsf")]

# generate playlist in sorted order
playlist = []
for minigsf in minigsfs:
    add_minigsf_to_playlist(minigsf, playlist)

print(json.dumps(playlist))

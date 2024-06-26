#!/usr/bin/env python3

import json
import platformdirs
import os.path
import sys

config_path = os.path.join(platformdirs.user_config_dir(), "agbplay.json")
profile_dir = os.path.join(os.path.join(platformdirs.user_config_dir(), "agbplay"), "profiles")

with open(config_path, "r") as config_file:
    config = json.load(config_file)

if "playlists" not in config:
    print("No playlists found in config")
    sys.exit(1)

for playlist in config["playlists"]:
    primary_gamecode = playlist["games"][0].replace(":", ".")
    profile_path = os.path.join(profile_dir, primary_gamecode + ".json")
    profile = {}

    def trans(dest_key, src_key):
        if src_key in playlist:
            edit[dest_key] = playlist[src_key]

    def trans_glob(dest_key, src_key):
        if src_key in config:
            edit[dest_key] = config[src_key]

    agbplay_sound_mode = {}
    edit = agbplay_sound_mode

    trans("accurateCh3Quantization", "accurate-ch3-quantization")
    trans("accurateCh3Volume", "accurate-ch3-volume")
    trans_glob("cgbPolyphony", "cgb-polyphony")
    trans("dmaBufferLen", "pcm-reverb-buffer-len")
    trans("emulateCgbSustainBug", "simulate-cgb-sustain-bug")
    trans_glob("maxLoopsExport", "max-loops-export")
    trans_glob("maxLoopsPlayback", "max-loops-playlist")
    trans_glob("padSilenceSecondsEnd", "pad-seconds-end")
    trans_glob("padSilenceSecondsStart", "pad-seconds-start")
    trans_glob("resamplerTypeFixed", "pcm-fixed-rate-resampling-algo")
    trans_glob("resamplerTypeNormal", "pcm-resampling-algo")
    trans("reverbType", "pcm-reverb-type")

    profile["agbplaySoundMode"] = agbplay_sound_mode

    table_idx = -1
    game_match = {}
    game_match["gameCodes"] = []
    for game in playlist["games"]:
        if len(game.split(":")) == 2:
            game, table_idx = game.split(":")
            table_idx = int(table_idx)
        game_match["gameCodes"].append(game)

    if table_idx >= 0:
        profile["songTableInfo"] = {"tableIdx" : table_idx}

    profile["gameMatch"] = game_match
    playlist_new = []

    if "songs" in playlist and playlist["songs"] != None:
        for song in playlist["songs"]:
            playlist_new.append({ "name" : song["name"], "id" : song["index"] })

    profile["playlist"] = playlist_new

    with open(profile_path, "w") as profile_file:
        json.dump(profile, profile_file, ensure_ascii=False, indent=2)

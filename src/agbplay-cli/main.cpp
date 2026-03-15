#include <argparse/argparse.hpp>

#include "Version.hpp"

int main(int argc, char *argv[]) {
    const std::string VER = GIT_VERSION_STRING;
    argparse::ArgumentParser program(argv[0], VER);

    // $ agbplay render
    argparse::ArgumentParser pRender("render", VER);
    pRender.add_description("Render song audio to files");
    program.add_subparser(pRender);

    // $ agbplay render master
    argparse::ArgumentParser pRenderMaster("master", VER);
    pRenderMaster.add_description("Render a master (stereo mix) file");
    pRender.add_subparser(pRenderMaster);

    // $ agbplay render stems
    argparse::ArgumentParser pRenderStems("stems", VER);
    pRenderStems.add_description("Render stems (multitrack) to files");
    pRender.add_subparser(pRenderStems);

    // $ agbplay song
    argparse::ArgumentParser pSong("song", VER);
    pSong.add_description("Inspect a song");
    program.add_subparser(pSong);

    // $ agbplay song show
    argparse::ArgumentParser pSongShow("show", VER);
    pSongShow.add_description("Show detailed song information");
    pSong.add_subparser(pSongShow);

    // $ agbplay song list
    argparse::ArgumentParser pSongList("list", VER);
    pSongList.add_description("List all available songs");
    pSong.add_subparser(pSongList);

    // $ agbplay songlist
    argparse::ArgumentParser pSonglist("songlist", VER);
    pSonglist.add_description("Inspect the list of available songs");
    program.add_subparser(pSonglist);

    // $ agbplay songlist show
    argparse::ArgumentParser pSonglistShow("show", VER);
    pSonglistShow.add_description("List all available songs (songlist only)");
    pSonglist.add_subparser(pSonglistShow);

    // $ agbplay songlist count
    argparse::ArgumentParser pSonglistCount("count", VER);
    pSonglistCount.add_description("Print the number of available songs");
    pSonglist.add_subparser(pSonglistCount);

    // $ agbplay playlist
    argparse::ArgumentParser pPlaylist("playlist", VER);
    pPlaylist.add_description("Inspect or edit the list in the playlist");
    program.add_subparser(pPlaylist);

    // $ agbplay playlist show
    argparse::ArgumentParser pPlaylistShow("show", VER);
    pPlaylistShow.add_description("List all available songs in the playlist");
    pPlaylist.add_subparser(pPlaylistShow);

    // $ agbplay playlist count
    argparse::ArgumentParser pPlaylistCount("count", VER);
    pPlaylistCount.add_description("Print the number of songs in the playlist");
    pPlaylist.add_subparser(pPlaylistCount);

    // $ agbplay playlist song
    argparse::ArgumentParser pPlaylistSong("song", VER);
    pPlaylistSong.add_description("Manipulate or inspect a song in the playlist");
    pPlaylist.add_subparser(pPlaylistSong);

    // $ agbplay playlist song add
    argparse::ArgumentParser pPlaylistSongAdd("add", VER);
    pPlaylistSongAdd.add_description("Add a song to the playlist");
    pPlaylistSong.add_subparser(pPlaylistSongAdd);

    // $ agbplay playlist song remove
    argparse::ArgumentParser pPlaylistSongRemove("remove", VER);
    pPlaylistSongRemove.add_description("Remove a song from the playlist");
    pPlaylistSong.add_subparser(pPlaylistSongRemove);

    // $ agbplay profile
    argparse::ArgumentParser pProfile("profile", VER);
    pProfile.add_description("Manipulate or inspect a profile");
    program.add_subparser(pProfile);

    // $ agbplay profile show
    argparse::ArgumentParser pProfileShow("show", VER);
    pProfileShow.add_description("Show detailed profile information");
    pProfile.add_subparser(pProfileShow);

    // $ agbplay profile list
    argparse::ArgumentParser pProfileList("list", VER);
    pProfileList.add_description("List available profiles");
    pProfile.add_subparser(pProfileList);

    // Parse
    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    // Determine command
    if (program.is_subcommand_used("render")) {
        if (pRender.is_subcommand_used("master")) {
        } else if (pRender.is_subcommand_used("stems")) {
        } else {
            std::cerr << pRender;
            return 1;
        }
    } else if (program.is_subcommand_used("song")) {
        if (pSong.is_subcommand_used("show")) {
        } else if (pSong.is_subcommand_used("list")) {
        } else {
            std::cerr << pSong;
            return 1;
        }
    } else if (program.is_subcommand_used("songlist")) {
        if (pSonglist.is_subcommand_used("show")) {
        } else if (pSonglist.is_subcommand_used("count")) {
        } else {
            std::cerr << pSonglist;
            return 1;
        }
    } else if (program.is_subcommand_used("playlist")) {
        if (pPlaylist.is_subcommand_used("show")) {
        } else if (pPlaylist.is_subcommand_used("count")) {
        } else if (pPlaylist.is_subcommand_used("song")) {
            if (pPlaylistSong.is_subcommand_used("add")) {
            } else if (pPlaylistSong.is_subcommand_used("remove")) {
            } else {
                std::cerr << pPlaylistSong;
                return 1;
            }
        } else {
            std::cerr << pPlaylist;
            return 1;
        }
    } else if (program.is_subcommand_used("profile")) {
        if (pProfile.is_subcommand_used("show")) {
        } else if (pProfile.is_subcommand_used("list")) {
        } else {
            std::cerr << pProfile;
            return 1;
        }
    } else {
        std::cerr << program;
        return 1;
    }
}

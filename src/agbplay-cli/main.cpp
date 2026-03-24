#include <functional>

#include <argparse/argparse.hpp>

#include "CLIPlaylist.hpp"
#include "CLIRender.hpp"
#include "CLISonglist.hpp"

#include "Version.hpp"
#include "Rom.hpp"
#include "Util.hpp"

namespace
{
    std::string songId;
    std::string outputPath;
    std::string songName;
    std::string playlistIdx;
}

int main(int argc, char *argv[])
{
    const std::string VER = GIT_VERSION_STRING;
    argparse::ArgumentParser program(argv[0], VER);

    std::string programImage;
    program.add_argument("image").store_into(programImage).help("ROM/GSF/ZIP of ROM to process");

    // $ agbplay render
    argparse::ArgumentParser pRender("render", VER);
    pRender.add_description("Render song audio to files");
    pRender.add_argument("song-id").store_into(songId).help("Song ID(s) to render. For multiple, separate with ';'").required();
    pRender.add_argument("output-path").store_into(outputPath).help("File path(s) to render to. For multiple, separate with ';'").required();
    program.add_subparser(pRender);

    // $ agbplay render master
    argparse::ArgumentParser pRenderMaster("master", VER);
    pRenderMaster.add_description("Render a master (mix) file");
    pRender.add_subparser(pRenderMaster);

    // $ agbplay render stems
    argparse::ArgumentParser pRenderStems("stems", VER);
    pRenderStems.add_description("Render stems (multitrack) to files");
    pRender.add_subparser(pRenderStems);

    // $ agbplay songlist
    argparse::ArgumentParser pSonglist("songlist", VER);
    pSonglist.add_description("Inspect the list of available songs by song ID");
    program.add_subparser(pSonglist);

    // $ agbplay songlist show
    argparse::ArgumentParser pSonglistShow("show", VER);
    pSonglistShow.add_description("Show detailed song information");
    pSonglistShow.add_argument("song-id").store_into(songId).help("Song ID to print details about").required();
    pSonglist.add_subparser(pSonglistShow);

    // $ agbplay songlist list
    argparse::ArgumentParser pSonglistList("list", VER);
    pSonglistList.add_description("List all available songs");
    pSonglist.add_subparser(pSonglistList);

    // $ agbplay songlist count
    argparse::ArgumentParser pSonglistCount("count", VER);
    pSonglistCount.add_description("Print the number of available songs");
    pSonglist.add_subparser(pSonglistCount);

    // $ agbplay playlist
    argparse::ArgumentParser pPlaylist("playlist", VER);
    pPlaylist.add_description("Inspect or edit the list in the playlist");
    program.add_subparser(pPlaylist);

    // $ agbplay playlist show
    argparse::ArgumentParser pPlaylistShow("list", VER);
    pPlaylistShow.add_description("List all available songs in the playlist");
    pPlaylist.add_subparser(pPlaylistShow);

    // $ agbplay playlist count
    argparse::ArgumentParser pPlaylistCount("count", VER);
    pPlaylistCount.add_description("Print the number of songs in the playlist");
    pPlaylist.add_subparser(pPlaylistCount);

    // $ agbplay playlist song
    argparse::ArgumentParser pPlaylistSong("song", VER);
    pPlaylistSong.add_description("Manipulate or inspect songs in playlist");
    pPlaylist.add_subparser(pPlaylistSong);

    // $ agbplay playlist song add
    argparse::ArgumentParser pPlaylistSongAdd("add", VER);
    pPlaylistSongAdd.add_description("Add a song to the playlist");
    pPlaylistSongAdd.add_argument("name").store_into(songName).help("Song name of the new playlist entry").required();
    pPlaylistSongAdd.add_argument("song-id").store_into(songId).help("Song ID of the new playlist entry").required();
    pPlaylistSongAdd.add_argument("playlist-idx").store_into(playlistIdx).help("Playlist index of the new entry").required();
    pPlaylistSong.add_subparser(pPlaylistSongAdd);

    // $ agbplay playlist song remove
    argparse::ArgumentParser pPlaylistSongRemove("remove", VER);
    pPlaylistSongRemove.add_description("Remove a song from the playlist");
    pPlaylistSongRemove.add_argument("playlist-idx").store_into(playlistIdx).help("Playlist index of the new entry").required();
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

    // Parse args.
    // We do not immediately raise an error here, since we try to obtain
    // the subcommand help text.
    bool parseError = false;
    std::string parseErrorMsg;
    auto parseErrorParser = std::cref(program);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& e) {
        parseError = true;
        parseErrorMsg = e.what();
    }

    std::function<void(void)> commandHandler;

    // Determine command
    if (program.is_subcommand_used("render")) {
        if (pRender.is_subcommand_used("master")) {
            commandHandler = std::bind(CLI::Render, songId, outputPath, false);
        } else if (pRender.is_subcommand_used("stems")) {
            commandHandler = std::bind(CLI::Render, songId, outputPath, true);
        } else {
            parseErrorParser = std::cref(pRender);
        }
    } else if (program.is_subcommand_used("songlist")) {
        if (pSonglist.is_subcommand_used("show")) {
            commandHandler = std::bind(CLI::SonglistShow, songId);
        } else if (pSonglist.is_subcommand_used("list")) {
            commandHandler = CLI::SonglistList;
        } else if (pSonglist.is_subcommand_used("count")) {
            commandHandler = CLI::SonglistCount;
        } else {
            parseErrorParser = std::cref(pSonglist);
        }
    } else if (program.is_subcommand_used("playlist")) {
        if (pPlaylist.is_subcommand_used("list")) {
            commandHandler = CLI::PlaylistList;
        } else if (pPlaylist.is_subcommand_used("count")) {
            commandHandler = CLI::PlaylistCount;
        } else if (pPlaylist.is_subcommand_used("song")) {
            if (pPlaylistSong.is_subcommand_used("add")) {
                commandHandler = std::bind(CLI::PlaylistSongAdd, songName, songId, playlistIdx);
            } else if (pPlaylistSong.is_subcommand_used("remove")) {
                commandHandler = std::bind(CLI::PlaylistSongRemove, playlistIdx);
            } else {
                parseErrorParser = std::cref(pPlaylistSong);
            }
        } else {
            parseErrorParser = std::cref(pPlaylist);
        }
    } else if (program.is_subcommand_used("profile")) {
        if (pProfile.is_subcommand_used("show")) {
            commandHandler = []() { throw std::runtime_error("This command is not yet implemented"); };
        } else if (pProfile.is_subcommand_used("list")) {
            commandHandler = []() { throw std::runtime_error("This command is not yet implemented"); };
        } else {
            parseErrorParser = std::cref(pProfile);
        }
    } else {
        parseErrorParser = std::cref(program);
    }

    // Actually handle parse errors or incomplete commands
    if (parseError) {
        std::cerr << parseErrorMsg << std::endl;
        std::cerr << parseErrorParser.get();
        return 1;
    } else if (!commandHandler) {
        std::cerr << parseErrorParser.get();
        return 1;
    } else {
        try {
            Rom::CreateInstance(StrToU8Str(program.get<std::string>("image")));

            commandHandler();
        } catch (const std::exception &e) {
            std::cerr << "Error:\n" << e.what() << std::endl;
            return 1;
        }
    }
}

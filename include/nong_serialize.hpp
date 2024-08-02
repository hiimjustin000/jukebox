#pragma once

#include <filesystem>
#include <fmt/core.h>
#include <matjson.hpp>
#include <memory>

#include "Geode/loader/Log.hpp"
#include "Geode/utils/Result.hpp"
#include "Geode/utils/string.hpp"
#include "nong.hpp"

template<>
struct matjson::Serialize<jukebox::SongIndexMetadata> {
    static geode::Result<jukebox::SongIndexMetadata> from_json(const matjson::Value& value) {
        if (!(value.contains("index_id") || !value["index_id"].is_string())) {
            return geode::Err("Invalid JSON key index_id");
        }
        if (!value.contains("song_id") || !value["song_id"].is_string()) {
            return geode::Err("Invalid JSON key song_id");
        }

        return geode::Ok(jukebox::SongIndexMetadata {
            value["index_id"].as_string(),
            value["song_id"].as_string(),
        });
    }

    static matjson::Value to_json(const jukebox::SongIndexMetadata& value) {
        matjson::Object ret = {};
        ret["index_id"] = value.m_indexID;
        ret["song_id"] = value.m_songID;
        return ret;
    }
};

template<>
struct matjson::Serialize<jukebox::SongMetadata> {
    static geode::Result<jukebox::SongMetadata> from_json(
        const matjson::Value& value,
        int songID
    ) {
        if (!(value.contains("name") || !value["name"].is_string())) {
            return geode::Err("Invalid JSON key name");
        }
        if (!value.contains("artist") || !value["artist"].is_string()) {
            return geode::Err("Invalid JSON key artist");
        }

        return geode::Ok(jukebox::SongMetadata {
            songID,
            value["name"].as_string(),
            value["artist"].as_string(),
            value.contains("level")
                && value["level"].is_string()
                    ? std::optional(value["level"].as_string())
                    : std::nullopt,
            value.contains("offset")
                && value["offset"].is_number()
                    ? value["offset"].as_int()
                    : 0
        });
    }
};

template<>
struct matjson::Serialize<jukebox::LocalSong> {
    static geode::Result<jukebox::LocalSong> from_json(
        const matjson::Value& value,
        int songID
    ) {
        auto metadata = matjson::Serialize<jukebox::SongMetadata>
            ::from_json(value, songID);

        if (metadata.isErr()) {
            return geode::Err(
                fmt::format(
                    "Local Song {} is invalid. Reason: {}",
                    value.dump(matjson::NO_INDENTATION),
                    metadata.unwrapErr()
                )
            );
        }

        if (!value.contains("path") || !value["path"].is_string()) {
            return geode::Err(
                "Local Song {} is invalid. Reason: invalid path",
                value.dump(matjson::NO_INDENTATION)
            );
        }

        return geode::Ok(jukebox::LocalSong {
            std::move(metadata.unwrap()),
            value["path"].as_string()
        });
    }

    static geode::Result<matjson::Value> to_json(const jukebox::LocalSong& value) {
        matjson::Object ret = {};
        ret["name"] = value.metadata()->m_name;
        ret["artist"] = value.metadata()->m_artist;
        #ifdef GEODE_IS_WINDOWS
        ret["path"] = geode::utils::string::wideToUtf8(value.path().c_str());
        #else
        ret["path"] = value.path().string();
        #endif
        ret["offset"] = value.metadata()->m_startOffset;
        if (value.metadata()->m_level.has_value()) {
            ret["level"] = value.metadata()->m_level.value();
        }
        return geode::Ok(ret);
    }
};

template<>
struct matjson::Serialize<jukebox::YTSong> {
    static geode::Result<jukebox::YTSong> from_json(
        const matjson::Value& value,
        int songID
    ) {
        auto metadata = matjson::Serialize<jukebox::SongMetadata>
            ::from_json(value, songID);

        if (metadata.isErr()) {
            return geode::Err(
                fmt::format(
                    "Local Song {} is invalid. Reason: {}",
                    value.dump(matjson::NO_INDENTATION),
                    metadata.unwrapErr()
                )
            );
        }

        if (!value.contains("path") || !value["path"].is_string()) {
            return geode::Err(
                "YT Song {} is invalid. Reason: invalid path",
                value.dump(matjson::NO_INDENTATION)
            );
        }

        if (!value.contains("youtube_id") || !value["youtube_id"].is_string()) {
            return geode::Err(
                "YT Song {} is invalid. Reason: invalid youtube ID",
                value.dump(matjson::NO_INDENTATION)
            );
        }

        if (value.contains("index") && !value["index"].is_object()) {
            return geode::Err(
                "YT Song {} is invalid. Reason: invalid index",
                value.dump(matjson::NO_INDENTATION)
            );
        }

        return geode::Ok(jukebox::YTSong {
            std::move(metadata.unwrap()),
            value["youtube_id"].as_string(),
            value.contains("index") ? std::optional(matjson::Serialize<jukebox::SongIndexMetadata>::from_json(value["index"]).unwrap()) : std::nullopt,
            value["path"].as_string(),
        });
    }

    static geode::Result<matjson::Value> to_json(const jukebox::YTSong& value) {
        if (!value.path().has_value()) {
            return geode::Err("YT song has no local path specified");
        }
        matjson::Object ret = {};
        ret["name"] = value.metadata()->m_name;
        ret["artist"] = value.metadata()->m_artist;
        ret["offset"] = value.metadata()->m_startOffset;
        ret["youtube_id"] = value.youtubeID();

        if (value.indexMetadata().has_value()) {
            ret["index"] = matjson::Serialize<jukebox::SongIndexMetadata>::to_json(*value.indexMetadata().value());
        }

        #ifdef GEODE_IS_WINDOWS
        ret["path"] = geode::utils::string::wideToUtf8(value.path().value().c_str());
        #else
        ret["path"] = value.path().value().string();
        #endif

        if (value.metadata()->m_level.has_value()) {
            ret["level"] = value.metadata()->m_level.value();
        }
        return geode::Ok(ret);
    }
};

template<>
struct matjson::Serialize<jukebox::HostedSong> {
    static geode::Result<jukebox::HostedSong> from_json(
        const matjson::Value& value,
        int songID
    ) {
        auto metadata = matjson::Serialize<jukebox::SongMetadata>
            ::from_json(value, songID);

        if (metadata.isErr()) {
            return geode::Err(
                fmt::format(
                    "Local Song {} is invalid. Reason: {}",
                    value.dump(matjson::NO_INDENTATION),
                    metadata.unwrapErr()
                )
            );
        }

        if (!value.contains("path") || !value["path"].is_string()) {
            return geode::Err(
                "Hosted Song {} is invalid. Reason: invalid path",
                value.dump(matjson::NO_INDENTATION)
            );
        }

        if (!value.contains("url") || !value["url"].is_string()) {
            return geode::Err(
                "Hosted Song {} is invalid. Reason: invalid url",
                value.dump(matjson::NO_INDENTATION)
            );
        }

        if (value.contains("index") && !value["index"].is_object()) {
            return geode::Err(
                "Hosted Song {} is invalid. Reason: invalid index",
                value.dump(matjson::NO_INDENTATION)
            );
        }

        return geode::Ok(jukebox::HostedSong{
            std::move(metadata.unwrap()),
            value["url"].as_string(),
            value.contains("index") ? std::optional(matjson::Serialize<jukebox::SongIndexMetadata>::from_json(value["index"]).unwrap()) : std::nullopt,
            value["path"].as_string()
        });
    }

    static geode::Result<matjson::Value> to_json(const jukebox::HostedSong& value) {
        if (!value.path().has_value()) {
            return geode::Err("HostedSong has no local path specified");
        }
        matjson::Object ret = {};
        ret["name"] = value.metadata()->m_name;
        ret["artist"] = value.metadata()->m_artist;
        ret["offset"] = value.metadata()->m_startOffset;
        ret["url"] = value.url();

        if (value.indexMetadata().has_value()) {
            ret["index"] = matjson::Serialize<jukebox::SongIndexMetadata>::to_json(*value.indexMetadata().value());
        }

        #ifdef GEODE_IS_WINDOWS
        ret["path"] = geode::utils::string::wideToUtf8(value.path().value().c_str());
        #else
        ret["path"] = value.path().value().string();
        #endif

        if (value.metadata()->m_level.has_value()) {
            ret["level"] = value.metadata()->m_level.value();
        }
        return geode::Ok(ret);
    }
};

template<>
struct matjson::Serialize<jukebox::Nongs> {
    static geode::Result<matjson::Value> to_json(
        jukebox::Nongs& value,
        bool includeDownloaded
    ) {
        matjson::Object ret = {};

        auto resDefault = matjson::Serialize<jukebox::LocalSong>::to_json(*value.defaultSong());
        if (resDefault.isErr()) {
            return geode::Err(resDefault.error());
        }
        ret["default"] = resDefault.ok();

        ret["active"] = value.active()->m_path;

        matjson::Array locals = {};
        for (auto& local : value.locals()) {
            auto res = matjson::Serialize<jukebox::LocalSong>::to_json(*local);
            if (res.isErr()) {
                return geode::Err(res.error());
            }
            locals.push_back(res.ok());
        }
        ret["locals"] = locals;

        matjson::Array youtubes = {};
        for (auto& youtube : value.youtube()) {
            auto res = matjson::Serialize<jukebox::YTSong>::to_json(*youtube);
            if (res.isErr()) {
                return geode::Err(res.error());
            }
            if (!includeDownloaded && !youtube->path().has_value()) {
                continue;
            }
            youtubes.push_back(res.ok());
        }
        ret["youtube"] = youtubes;

        matjson::Array hosteds = {};
        for (auto& hosted : value.hosted()) {
            auto res = matjson::Serialize<jukebox::HostedSong>::to_json(*hosted);
            if (res.isErr()) {
                return geode::Err(res.error());
            }
            if (!includeDownloaded && !hosted->path().has_value()) {
                continue;
            }
            hosteds.push_back(res.ok());
        }
        ret["hosted"] = hosteds;

        return geode::Ok(ret);
    }

    static geode::Result<jukebox::Nongs> from_json(
        const matjson::Value& value,
        int songID
    ) {
        if (!value.contains("default") || !value["default"].is_object()) {
            return geode::Err("Invalid nongs object for id {}", songID);
        }

        auto defaultSong = matjson::Serialize<jukebox::LocalSong>
            ::from_json(value["default"].as_object(), songID);

        if (defaultSong.isErr()) {
            // TODO think about something
            // Try recwovery if the song ID has some nongs
            return geode::Err("womp womp");
        }

        std::filesystem::path active;

        bool foundActive = false;

        jukebox::Nongs nongs = {
            songID,
            std::move(defaultSong.unwrap())
        };

        // If somehow active is invalid, just get the default path
        if (!value.contains("active") || !value["active"].is_string()) {
            active = defaultSong.unwrap().path();
            // This cannot fail, since we just set it as active
            auto _ = nongs.setActive(active);
            foundActive = true;
        } else {
            active = value["active"].as_string();
        }

        if (value.contains("locals") && value["locals"].is_array()) {
            for (auto& local : value["locals"].as_array()) {
                auto res = matjson::Serialize<jukebox::LocalSong>
                    ::from_json(local, songID);

                if (res.isErr()) {
                    geode::log::error("{}", res.unwrapErr());
                    continue;
                }

                auto song = res.unwrap();

                nongs.locals()
                    .push_back(
                        std::make_unique<jukebox::LocalSong>(song)
                    );

                if (song.path() == active) {
                    foundActive = true;
                    auto _ = nongs.setActive(song.path());
                }
            }
        }

        if (value.contains("youtube") && value["youtube"].is_array()) {
            for (auto& yt : value["youtube"].as_array()) {
                auto res = matjson::Serialize<jukebox::YTSong>
                    ::from_json(yt, songID);

                if (res.isErr()) {
                    geode::log::error("{}", res.unwrapErr());
                    continue;
                }

                auto song = res.unwrap();

                nongs.youtube()
                    .push_back(
                        std::make_unique<jukebox::YTSong>(song)
                    );

                if (
                    song.path().has_value()
                    && song.path().value() == active
                ) {
                    foundActive = true;
                    auto _ = nongs.setActive(song.path().value());
                }
            }
        }

        if (value.contains("hosted") && value["hosted"].is_array()) {
            for (auto& hosted : value["hosted"].as_array()) {
                auto res = matjson::Serialize<jukebox::HostedSong>
                    ::from_json(hosted, songID);

                if (res.isErr()) {
                    geode::log::error("{}", res.unwrapErr());
                }

                auto song = res.unwrap();

                nongs.hosted()
                    .push_back(
                        std::make_unique<jukebox::HostedSong>(song)
                    );

                if (
                    song.path().has_value()
                    && song.path().value() == active
                ) {
                    foundActive = true;
                    auto _ = nongs.setActive(song.path().value());
                }
            }
        }

        if (!foundActive) {
            auto _ = nongs.setActive(defaultSong.unwrap().path());
        }

        return geode::Ok(std::move(nongs));
    }
};

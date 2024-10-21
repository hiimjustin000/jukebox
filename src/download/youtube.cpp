#include "download/youtube.hpp"

#include <string>

#include "Geode/utils/Result.hpp"
#include "Geode/utils/general.hpp"
#include "Geode/utils/web.hpp"

#include "download/download.hpp"
#include "download/hosted.hpp"
#include "utils/task.hpp"

using namespace geode::prelude;

web::WebTask getMetadata(const std::string& id);
Result<std::string> getUrlFromMetadataPayload(web::WebResponse* resp);
jukebox::download::DownloadTask onMetadata(web::WebResponse*);

namespace jukebox {

namespace download {

DownloadTask startYoutubeDownload(const std::string& id) {
    if (id.length() != 11) {
        return DownloadTask::immediate(Err("Invalid YouTube ID"));
    }

    return jukebox::switchMap<web::WebResponse, web::WebProgress,
                              Result<ByteVector>, float>(
        getMetadata(id), [](web::WebResponse* r) { return onMetadata(r); });
}

}  // namespace download

}  // namespace jukebox

Result<std::string> getUrlFromMetadataPayload(web::WebResponse* r) {
    if (!r->ok()) {
        return Err(fmt::format(
            "cobalt metadata query failed with status code {}", r->code()));
    }

    Result<matjson::Value> jsonRes = r->json();
    if (jsonRes.isErr()) {
        return Err("cobalt metadata query returned invalid JSON");
    }

    matjson::Value payload = jsonRes.unwrap();
    if (!payload.contains("status") || !payload["status"].is_string() ||
        payload["status"].as_string() != "stream") {
        return Err("Invalid metadata status");
    }

    if (!payload.contains("url") || !payload["url"].is_string()) {
        return Err("No download URL returned");
    }

    return Ok(payload["url"].as_string());
}

web::WebTask getMetadata(const std::string& id) {
    return web::WebRequest()
        .timeout(std::chrono::seconds(30))
        .bodyJSON(matjson::Object{
            {"url", fmt::format("https://www.youtube.com/watch?v={}", id)},
            {"aFormat", "mp3"},
            {"isAudioOnly", "true"}})
        .header("Accept", "application/json")
        .header("Content-Type", "application/json")
        .post("https://api.cobalt.tools/api/json");
}

jukebox::download::DownloadTask onMetadata(web::WebResponse* result) {
    Result<std::string> res = getUrlFromMetadataPayload(result);
    if (res.isErr()) {
        return jukebox::download::DownloadTask::immediate(Err(res.error()));
    }

    return jukebox::download::startHostedDownload(res.unwrap());
}
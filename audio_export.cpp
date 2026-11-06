#include "audio_export.h"
#include "audio_engine.h"
#include <QFile>
#include <QDataStream>
#include <algorithm>
#include <vector>

#ifdef HAVE_LAME
#include <lame/lame.h>
#endif
#ifdef HAVE_VORBIS
#include <vorbis/vorbisenc.h>
#endif

bool writeWav(const QString& path, const std::vector<float>& data) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    QDataStream ds(&f);
    ds.setByteOrder(QDataStream::LittleEndian);

    const uint32_t dataSize = data.size() * 2;
    const uint32_t riffSize = 36 + dataSize;
    ds.writeRawData("RIFF", 4);
    ds << riffSize;
    ds.writeRawData("WAVE", 4);
    ds.writeRawData("fmt ", 4);
    ds << (uint32_t)16 << (uint16_t)1 << (uint16_t)1
       << (uint32_t)SR << (uint32_t)(SR * 2) << (uint16_t)2 << (uint16_t)16;
    ds.writeRawData("data", 4);
    ds << dataSize;
    for (float s : data) {
        int16_t v = (int16_t)std::clamp(s * 32767.0f, -32768.0f, 32767.0f);
        ds << v;
    }
    return true;
}

#ifdef HAVE_LAME
bool writeMp3(const QString& path, const std::vector<float>& data) {
    lame_global_flags* gfp = lame_init();
    lame_set_in_samplerate(gfp, SR);
    lame_set_num_channels(gfp, 1);
    lame_set_brate(gfp, 192);
    lame_set_quality(gfp, 2);
    lame_init_params(gfp);

    std::vector<short> pcm(data.size());
    for (size_t i = 0; i < data.size(); ++i)
        pcm[i] = (short)std::clamp(data[i] * 32767.0f, -32768.0f, 32767.0f);

    std::vector<unsigned char> mp3buf(pcm.size() * 5 / 4 + 7200);
    int n = lame_encode_buffer(gfp, pcm.data(), nullptr, (int)pcm.size(),
                               mp3buf.data(), (int)mp3buf.size());
    std::vector<unsigned char> tail(7200);
    int t = lame_encode_flush(gfp, tail.data(), (int)tail.size());

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) { lame_close(gfp); return false; }
    f.write((char*)mp3buf.data(), n);
    f.write((char*)tail.data(), t);
    lame_close(gfp);
    return true;
}
#else
bool writeMp3(const QString&, const std::vector<float>&) { return false; }
#endif

#ifdef HAVE_VORBIS
bool writeOgg(const QString& path, const std::vector<float>& data) {
    vorbis_info vi; vorbis_info_init(&vi);
    if (vorbis_encode_init_vbr(&vi, 1, SR, 0.5f)) { vorbis_info_clear(&vi); return false; }

    vorbis_comment vc; vorbis_comment_init(&vc);
    vorbis_comment_add_tag(&vc, "ENCODER", "sfx-qt");

    vorbis_dsp_state vd; vorbis_block vb;
    vorbis_analysis_init(&vd, &vi);
    vorbis_block_init(&vd, &vb);

    ogg_stream_state os;
    ogg_stream_init(&os, 42);

    ogg_packet header, header_comm, header_code;
    vorbis_analysis_headerout(&vd, &vc, &header, &header_comm, &header_code);
    ogg_stream_packetin(&os, &header);
    ogg_stream_packetin(&os, &header_comm);
    ogg_stream_packetin(&os, &header_code);

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        vorbis_block_clear(&vb); vorbis_dsp_clear(&vd);
        vorbis_comment_clear(&vc); vorbis_info_clear(&vi);
        ogg_stream_clear(&os);
        return false;
    }

    ogg_page og;
    while (ogg_stream_flush(&os, &og)) {
        f.write((char*)og.header, og.header_len);
        f.write((char*)og.body, og.body_len);
    }

    const int CHUNK = 1024;
    int pos = 0;
    while (pos < (int)data.size()) {
        int cnt = std::min(CHUNK, (int)data.size() - pos);
        float** buf = vorbis_analysis_buffer(&vd, cnt);
        for (int i = 0; i < cnt; ++i) buf[0][i] = data[pos + i];
        vorbis_analysis_wrote(&vd, cnt);
        pos += cnt;
        while (vorbis_analysis_blockout(&vd, &vb) == 1) {
            vorbis_analysis(&vb, nullptr);
            vorbis_bitrate_addblock(&vb);
            ogg_packet op;
            while (vorbis_bitrate_flushpacket(&vd, &op)) {
                ogg_stream_packetin(&os, &op);
                while (ogg_stream_pageout(&os, &og)) {
                    f.write((char*)og.header, og.header_len);
                    f.write((char*)og.body, og.body_len);
                }
            }
        }
    }
    vorbis_analysis_wrote(&vd, 0);
    while (vorbis_analysis_blockout(&vd, &vb) == 1) {
        vorbis_analysis(&vb, nullptr);
        vorbis_bitrate_addblock(&vb);
        ogg_packet op;
        while (vorbis_bitrate_flushpacket(&vd, &op)) {
            ogg_stream_packetin(&os, &op);
            while (ogg_stream_pageout(&os, &og)) {
                f.write((char*)og.header, og.header_len);
                f.write((char*)og.body, og.body_len);
            }
        }
    }
    while (ogg_stream_flush(&os, &og)) {
        f.write((char*)og.header, og.header_len);
        f.write((char*)og.body, og.body_len);
    }
    vorbis_block_clear(&vb); vorbis_dsp_clear(&vd);
    vorbis_comment_clear(&vc); vorbis_info_clear(&vi);
    ogg_stream_clear(&os);
    return true;
}
#else
bool writeOgg(const QString&, const std::vector<float>&) { return false; }
#endif
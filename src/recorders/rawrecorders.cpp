/*
    qarv, a Qt interface to aravis.
    Copyright (C) 2013 Jure Varlec <jure.varlec@ad-vega.si>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "recorders/rawrecorders.h"
#include <QFile>
#include <QSettings>
#include <QRegExp>
#include <QFileInfo>
#include <QDebug>
extern "C" {
#include <libavutil/pixdesc.h>
}

using namespace QArv;

static const QString descExt(".desc");
static const QRegExp descExtRegexp("\\.desc$");

void initDescfile(QSettings& s, QSize size, int FPS) {
  s.beginGroup("qarv_raw_video_description");
  s.remove("");
  s.setValue("description_version", "0.1");
  QFileInfo finfo(s.fileName());
  QString fname(finfo.baseName().remove(descExtRegexp));
  s.setValue("file_name", fname);
  s.setValue("frame_size", size);
  s.setValue("nominal_fps", FPS);
}

class RawUndecoded: public Recorder {
public:
  RawUndecoded(QArvDecoder* decoder_,
               QString fileName,
               QSize size,
               int FPS,
               bool appendToFile) :
    file(fileName), decoder(decoder_) {
    file.open(appendToFile ? QIODevice::Append : QIODevice::WriteOnly);
    if (isOK() && !appendToFile) {
      QSettings s(fileName + descExt, QSettings::Format::IniFormat);
      initDescfile(s, size, FPS);
      s.setValue("encoding_type", "aravis");
      auto pxfmt = QString::number(decoder->pixelFormat(), 16);
      s.setValue("arv_pixel_format", QString("0x") + pxfmt);
    }
  }

  bool isOK() {
    return file.isOpen() && (file.error() == QFile::NoError);
  }

  void recordFrame(QByteArray raw, cv::Mat decoded) {
    if (isOK())
      file.write(raw);
  }

private:
  QFile file;
  QArvDecoder* decoder;
};

class RawDecoded8: public Recorder {
public:
  RawDecoded8(QArvDecoder* decoder_,
              QString fileName,
              QSize size,
              int FPS,
              bool appendToFile) :
    file(fileName), decoder(decoder_), OK(true) {
    file.open(appendToFile ? QIODevice::Append : QIODevice::WriteOnly);
    if (isOK() && !appendToFile) {
      enum PixelFormat fmt;
      switch (decoder->cvType()) {
      case CV_8UC1:
      case CV_16UC1:
        fmt = PIX_FMT_GRAY8;
        temporary.create(size.height(), size.width(), CV_8UC1);
        break;
      case CV_8UC3:
      case CV_16UC3:
        fmt = PIX_FMT_BGR24;
        temporary.create(size.height(), size.width(), CV_8UC3);
        break;
      default:
        qDebug() << "Recorder: Invalid CV image format";
        return;
      }
      QSettings s(fileName + descExt, QSettings::Format::IniFormat);
      initDescfile(s, size, FPS);
      s.setValue("encoding_type", "libavutil");
      s.setValue("libavutil_pixel_format", fmt);
      s.setValue("libavutil_pixel_format_name", av_get_pix_fmt_name(fmt));
    }
  }

  bool isOK() {
    return OK && file.isOpen() && (file.error() == QFile::NoError);
  }

  void recordFrame(QByteArray raw, cv::Mat decoded) {
    if (!decoded.isContinuous()) {
      qDebug() << "Image is not continuous!";
      OK = false;
    }
    if (!isOK())
      return;
    if (decoded.depth() == CV_8U) {
      file.write(reinterpret_cast<char*>(decoded.data),
                 decoded.total()*decoded.elemSize1());
    } else {
      auto out = temporary.ptr<uint8_t>(0);
      for (auto in = decoded.begin<uint16_t>(), end = decoded.end<uint16_t>();
           in != end; ) {
        *out = (*in) >> 8;
        out++;
        in++;
      }
      file.write(reinterpret_cast<char*>(temporary.data),
                 temporary.total()*temporary.elemSize1());
    }
  }

private:
  QFile file;
  QArvDecoder* decoder;
  bool OK;
  cv::Mat temporary;
};

class RawDecoded16: public Recorder {
public:
  RawDecoded16(QArvDecoder* decoder_,
               QString fileName,
               QSize size,
               int FPS,
               bool appendToFile) :
    file(fileName), decoder(decoder_), OK(true) {
    file.open(appendToFile ? QIODevice::Append : QIODevice::WriteOnly);
    if (isOK() && !appendToFile) {
      enum PixelFormat fmt;
      switch (decoder->cvType()) {
      case CV_8UC1:
      case CV_16UC1:
        fmt = PIX_FMT_GRAY16;
        temporary.create(size.height(), size.width(), CV_16UC1);
        break;
      case CV_8UC3:
      case CV_16UC3:
        fmt = PIX_FMT_BGR48;
        temporary.create(size.height(), size.width(), CV_16UC3);
        break;
      default:
        qDebug() << "Recorder: Invalid CV image format";
        return;
      }
      QSettings s(fileName + descExt, QSettings::Format::IniFormat);
      initDescfile(s, size, FPS);
      s.setValue("encoding_type", "libavutil");
      s.setValue("libavutil_pixel_format", fmt);
      s.setValue("libavutil_pixel_format_name", av_get_pix_fmt_name(fmt));
    }
  }

  bool isOK() {
    return OK && file.isOpen() && (file.error() == QFile::NoError);
  }

  void recordFrame(QByteArray raw, cv::Mat decoded) {
    if (!decoded.isContinuous()) {
      qDebug() << "Image is not continuous!";
      OK = false;
    }
    if (!isOK())
      return;
    if (decoded.depth() == CV_16U) {
      file.write(reinterpret_cast<char*>(decoded.data),
                 decoded.total()*decoded.elemSize1());
    } else {
      auto out = temporary.ptr<uint16_t>(0);
      for (auto in = decoded.begin<uint8_t>(), end = decoded.end<uint8_t>();
           in != end;) {
        *out = (*in) << 8;
        out++;
        in++;
      }
      file.write(reinterpret_cast<char*>(temporary.data),
                 temporary.total()*temporary.elemSize1());
    }
  }

private:
  QFile file;
  QArvDecoder* decoder;
  bool OK;
  cv::Mat temporary;
};

Recorder* RawUndecodedFormat::makeRecorder(QArvDecoder* decoder,
                                           QString fileName,
                                           QSize frameSize,
                                           int framesPerSecond,
                                           bool appendToFile) {
  return new RawUndecoded(decoder, fileName, frameSize, framesPerSecond, appendToFile);
}

Recorder* RawDecoded8Format::makeRecorder(QArvDecoder* decoder,
                                          QString fileName,
                                          QSize frameSize,
                                          int framesPerSecond,
                                          bool appendToFile) {
  return new RawDecoded8(decoder, fileName, frameSize, framesPerSecond, appendToFile);
}

Recorder* RawDecoded16Format::makeRecorder(QArvDecoder* decoder,
                                           QString fileName,
                                           QSize frameSize,
                                           int framesPerSecond,
                                           bool appendToFile) {
  return new RawDecoded16(decoder, fileName, frameSize, framesPerSecond, appendToFile);
}

Q_EXPORT_PLUGIN2(RawUndecoded, QArv::RawUndecodedFormat)
Q_IMPORT_PLUGIN(RawUndecoded)

Q_EXPORT_PLUGIN2(RawDecoded8, QArv::RawDecoded8Format)
Q_IMPORT_PLUGIN(RawDecoded8)

Q_EXPORT_PLUGIN2(RawDecoded16, QArv::RawDecoded16Format)
Q_IMPORT_PLUGIN(RawDecoded16)
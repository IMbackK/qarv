/*
    qarv, a Qt interface to aravis.
    Copyright (C) 2012  Jure Varlec <jure.varlec@ad-vega.si>

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

#ifndef MONO8_H
#define MONO8_H

#include "api/qarvdecoder.h"
extern "C" {
  #include <arvenums.h>
}

namespace QArv {

class Mono8Decoder : public QArvDecoder {
public:
  Mono8Decoder(QSize size_);
  void decode(QByteArray frame);
  QImage getQImage();
  cv::Mat getCvImage();

private:
  QSize size;
  cv::Mat M;
};

class Mono8Format : public QObject, public QArvPixelFormat {
  Q_OBJECT
  Q_INTERFACES(QArvPixelFormat)

public:
  ArvPixelFormat pixelFormat() { return ARV_PIXEL_FORMAT_MONO_8; }
  QArvDecoder* makeDecoder(QSize size) { return new Mono8Decoder(size); }
};

}

#endif
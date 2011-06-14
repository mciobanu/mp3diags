/***************************************************************************
 *   MP3 Diags - diagnosis, repairs and tag editing for MP3 files          *
 *                                                                         *
 *   Copyright (C) 2009 by Marian Ciobanu                                  *
 *   ciobi@inbox.com                                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/***************************************************************************
 *   Portable mode utilities                                               *
 *   Copyright (C) 2011 by Aluísio Augusto Silva Gonçalves                 *
 *   kalug@users.sourceforge.net                                           *
 ***************************************************************************/


#include "PortableMode.h"

/*const char* getenv1(const char*) {
    return "/home/ciobi/cpp/Mp3Utils/mp3diags/trunk/mp3diags/portable_tst3";
}*/


bool PortableMode::enabled () {
    return !QString::fromLocal8Bit(getenv("MP3DIAGS_CONFIG_PATH")).isEmpty();
}

QDir PortableMode::configDir () {
    //if (!enabled()) return NULL; //doesn't compile on openSUSE 11.2 (GCC 4.4.1) or MinGW 3.4 or MinGW 4.4 (and probably other environments)
    if (!enabled()) return QString();
    QDir configDir (QString::fromLocal8Bit(getenv("MP3DIAGS_CONFIG_PATH")));
    if (!configDir.exists()) {
        // configDir.mkdir("."); // !!! this doesn't work in openSUSE 11.2 (and probably other environments)
        configDir.mkpath(".");
    }
    return configDir;
}

QDir PortableMode::relativePath (QString fileName, bool createDir) {
    //if (!enabled()) return NULL; //doesn't compile on openSUSE 11.2 (GCC 4.4.1) or MinGW 3.4 or MinGW 4.4 (and probably other environments)
    if (!enabled()) return QString();
    QDir child (configDir().absoluteFilePath(fileName));
    //qDebug("child=%s (%s) (%s)", child.path().toUtf8().constData(), child.absolutePath().toUtf8().constData(), child.canonicalPath().toUtf8().constData());
    if (createDir && !child.exists()) {
        //child.mkdir("."); // this doesn't work in openSUSE 11.2 (and probably other environments)
        child.mkpath(".");
    }
    return child;
}

QString PortableMode::getRelativeSessionPath (QString path) {
    if (enabled() && (relativePath("sessions") == QFileInfo(path).dir())) {
        return relativePath("sessions").relativeFilePath(path);
    } else {
        return path;
    }
}

QString PortableMode::getAbsoluteSessionPath (QString path) {
    if (enabled() && QFileInfo(path).isRelative()) {
        return QDir::cleanPath(relativePath("sessions", true).absoluteFilePath(path));
    } else {
        return path;
    }
}
/*
namespace {

struct A {
    A(int) {}
};


struct B {
    B(A) {}
};

void f(B) {
}

void g() {
    f(4);
    f(B(5));
}

}
*/


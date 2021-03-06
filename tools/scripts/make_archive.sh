#!/bin/sh

#############################################################################
#
# Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
# Contact: http://www.qt-project.org/legal
#
# This file is part of the QtWebEngine module of the Qt Toolkit.
#
# $QT_BEGIN_LICENSE:LGPL$
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and Digia.  For licensing terms and
# conditions see http://qt.digia.com/licensing.  For further information
# use the contact form at http://qt.digia.com/contact-us.
#
# GNU Lesser General Public License Usage
# Alternatively, this file may be used under the terms of the GNU Lesser
# General Public License version 2.1 as published by the Free Software
# Foundation and appearing in the file LICENSE.LGPL included in the
# packaging of this file.  Please review the following information to
# ensure the GNU Lesser General Public License version 2.1 requirements
# will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
#
# In addition, as a special exception, Digia gives you certain additional
# rights.  These rights are described in the Digia Qt LGPL Exception
# version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3.0 as published by the Free Software
# Foundation and appearing in the file LICENSE.GPL included in the
# packaging of this file.  Please review the following information to
# ensure the GNU General Public License version 3.0 requirements will be
# met: http://www.gnu.org/copyleft/gpl.html.
#
#
# $QT_END_LICENSE$
#
#############################################################################


if [ $# -ne 2 ]; then
    echo "Usage: $0 git-ref release-name"
    echo "       example: $0 origin/master qtwebengine-opensource-src-0.1.0-tp1"
    exit 0
fi

QTWEBENGINE_REF=$1
RELEASE_NAME=$2
OUTDIR=`pwd`

THIRD_PARTY_REF=`git log -p $QTWEBENGINE_REF --ignore-submodules=none -n 1 -- src/3rdparty | grep "+Subproject commit" | cut -f 3 -d ' '`

git archive $QTWEBENGINE_REF --format tar --prefix=$RELEASE_NAME/ -o $OUTDIR/$RELEASE_NAME.tar
cd src/3rdparty
git archive $THIRD_PARTY_REF --format tar --prefix=$RELEASE_NAME/src/3rdparty/ -o $OUTDIR/$RELEASE_NAME.src.3rdparty.tar

tar --concatenate --file=$OUTDIR/$RELEASE_NAME.tar $OUTDIR/$RELEASE_NAME.src.3rdparty.tar
rm $OUTDIR/$RELEASE_NAME.src.3rdparty.tar

gzip $OUTDIR/$RELEASE_NAME.tar


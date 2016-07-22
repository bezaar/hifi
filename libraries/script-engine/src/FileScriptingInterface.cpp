//
//  FileScriptingInterface.cpp
//  libraries/script-engine/src
//
//  Created by Elisa Lupin-Jimenez on 6/28/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QBuffer>
#include <QTextCodec>
#include <QIODevice>
#include <QUrl>
#include <QByteArray>
#include <QString>
#include <QFileInfo>
#include <quazip5/quazip.h>
#include <quazip5/JlCompress.h>
#include "ResourceManager.h"

#include "FileScriptingInterface.h"


FileScriptingInterface::FileScriptingInterface(QObject* parent) : QObject(parent) {
	// nothing for now
}

void FileScriptingInterface::runUnzip(QString path, QUrl url) {
	qDebug() << "Url that was downloaded: " + url.toString();
	qDebug() << "Path where download is saved: " + path;
	unzipFile(path);
}

bool FileScriptingInterface::testUrl(QUrl url) {
	if (url.toString().contains(".zip")) return true;
	return false;
}

QString FileScriptingInterface::getTempDir() {
	QTemporaryDir dir;
	dir.setAutoRemove(false);
	return dir.path();
	// remember I must do something to delete this temp dir later
}

QString FileScriptingInterface::convertUrlToPath(QUrl url) {
	QString newUrl;
	QString oldUrl = url.toString();
	newUrl = oldUrl.section("filename=", 1, 1);
	qDebug() << "Filename should be: " + newUrl;
	return newUrl;
}

void FileScriptingInterface::downloadZip(QString path, const QString link) {
	QUrl url = QUrl(link);
	auto request = ResourceManager::createResourceRequest(nullptr, url);
	connect(request, &ResourceRequest::finished, this, [this, path]{
		unzipFile(path);
	});
    request->send();
}

// clement's help :D
void FileScriptingInterface::unzipFile(QString path) {
	//ResourceRequest* request = qobject_cast<ResourceRequest*>(sender());
    //QUrl url = request->getUrl();

    //if (request->getResult() == ResourceRequest::Success) {
        qDebug() << "Zip file was downloaded";
	    QDir dir(path);
        //QByteArray compressedFileContent = request->getData(); // <-- Downloaded file is in here
        //QBuffer buffer(&compressedFileContent);
        //buffer.open(QIODevice::ReadOnly);

		//QString zipFileName = QFile::decodeName(compressedFileContent);
        QString dirName = dir.path();
		QDir testPath("C:/Users/elisa/Downloads/banana.zip");
		qDebug() << "Zip directory is stored at: " + dirName;
        QStringList list = JlCompress::extractDir(testPath.path(), "C:/Users/elisa/Downloads/test");

		qDebug() << list;

        //QFileInfoList files = dir.entryInfoList();
		QFileInfoList files = testPath.entryInfoList();
		foreach(QFileInfo file, files) {
			qDebug() << "My file: " + file.fileName();
			recursiveFileScan(file, &dirName);
			
        }


        /*foreach (QFileInfo file, files) {
        	if (file.isDir()) {
        		if (file.fileName().contains(".zip")) {
        			qDebug() << "Zip file expanded: " + file.fileName();
        		}

        		qDebug() << "Regular file logged: " + file.fileName();
        	}
        }*/


        

        //QFile file = 
        //need to convert this byte array to a file
        /*QuaZip zip(zipFileName);

        if (zip.open(QuaZip::mdUnzip)) {
        qDebug() << "Opened";
 
        for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile()) {
            // do something
            qDebug() << zip.getCurrentFileName();
        }
        if (zip.getZipError() == UNZ_OK) {
            // ok, there was no error
        }*/


        //buffer.close();
    //} else {
       // qDebug() << "Could not download the zip file";
    //}

}

void FileScriptingInterface::recursiveFileScan(QFileInfo file, QString* dirName) {
	/*if (!file.isDir()) {
		qDebug() << "Regular file logged: " + file.fileName();
		return;
	}*/
    QFileInfoList files;

	if (file.fileName().contains(".zip")) {
		qDebug() << "Extracting archive: " + file.fileName();
		JlCompress::extractDir(file.fileName());
    }
    files = file.dir().entryInfoList();

	/*if (files.empty()) {
		files = JlCompress::getFileList(file.fileName());
	}*/

	foreach (QFileInfo file, files) {
        qDebug() << "Looking into file: " + file.fileName();
        recursiveFileScan(file, dirName);
	}
    return;
}

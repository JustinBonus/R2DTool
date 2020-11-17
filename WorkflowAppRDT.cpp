/* *****************************************************************************
Copyright (c) 2016-2017, The Regents of the University of California (Regents).
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.

REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED HEREUNDER IS 
PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, 
UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

*************************************************************************** */

// Written by: Stevan Gavrilovic
// Latest revision: 10.08.2020

#include "AssetsWidget.h"
#include "HazardsWidget.h"
#include "VisualizationWidget.h"

// Test
#include "shakeMapClient.h"

#include "WorkflowAppRDT.h"
#include "LocalApplication.h"
#include "RemoteApplication.h"
#include "RemoteJobManager.h"
#include "RunWidget.h"
#include "InputWidgetUQ.h"
#include "RunLocalWidget.h"
#include "RemoteService.h"
#include "SimCenterComponentSelection.h"
#include "GeneralInformationWidget.h"
#include "RandomVariablesContainer.h"
#include "InputWidgetSampling.h"
#include "DakotaResultsSampling.h"
#include "CustomizedItemModel.h"
#include "GoogleAnalytics.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QHostInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QStackedWidget>
#include <QUuid>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

// static pointer for global procedure set in constructor
static WorkflowAppRDT *theApp = nullptr;

// global procedure
int getNumParallelTasks()
{
    return theApp->getMaxNumParallelTasks();
}

WorkflowAppRDT::WorkflowAppRDT(RemoteService *theService, QWidget *parent)
    : WorkflowAppWidget(theService, parent)
{
    // set static pointer for global procedure
    theApp = this;

    //
    // create the various widgets
    //

    client = new shakeMapClient();

    theRVs = new RandomVariablesContainer();
    theVisualizationWidget = new VisualizationWidget(this);
    theAssetsWidget = new AssetsWidget(this,theVisualizationWidget);
    theHazardsWidget = new HazardsWidget(this, theVisualizationWidget, theRVs);

    //    theGI = GeneralInformationWidget::getInstance();
    //    theSIM = new SIM_Selection(theRVs);
    //    theEventSelection = new EarthquakeEventSelection(theRVs, theGI);
    //    theAnalysisSelection = new FEM_Selection(theRVs);
    theUQ_Selection = new UQ_EngineSelection(theRVs);
    //    theEDP_Selection = new EDP_EarthquakeSelection(theRVs);
    theResults = theUQ_Selection->getResults();

    localApp = new LocalApplication("femUQ.py");
    remoteApp = new RemoteApplication("femUQ.py", theService);

    theJobManager = new RemoteJobManager(theService);

    SimCenterWidget *theWidgets[1];
    theRunWidget = new RunWidget(localApp, remoteApp, theWidgets, 0);

    // connect signals and slots - error messages and signals
    //    connect(theResults,SIGNAL(sendStatusMessage(QString)), this,SLOT(statusMessage(QString)));
    //    connect(theResults, SIGNAL(sendErrorMessage(QString)), this, SLOT(errorMessage(QString)));

    //    connect(theGI,SIGNAL(sendErrorMessage(QString)), this,SLOT(errorMessage(QString)));
    //    connect(theGI,SIGNAL(sendStatusMessage(QString)), this,SLOT(statusMessage(QString)));
    //    connect(theGI,SIGNAL(sendFatalMessage(QString)), this,SLOT(fatalMessage(QString)));


    connect(theRunWidget,SIGNAL(sendErrorMessage(QString)), this,SLOT(errorMessage(QString)));
    connect(theRunWidget,SIGNAL(sendStatusMessage(QString)), this,SLOT(statusMessage(QString)));
    connect(theRunWidget,SIGNAL(sendFatalMessage(QString)), this,SLOT(fatalMessage(QString)));

    connect(localApp,SIGNAL(sendErrorMessage(QString)), this,SLOT(errorMessage(QString)));
    connect(localApp,SIGNAL(sendStatusMessage(QString)), this,SLOT(statusMessage(QString)));
    connect(localApp,SIGNAL(sendFatalMessage(QString)), this,SLOT(fatalMessage(QString)));

    connect(remoteApp,SIGNAL(sendErrorMessage(QString)), this,SLOT(errorMessage(QString)));
    connect(remoteApp,SIGNAL(sendStatusMessage(QString)), this,SLOT(statusMessage(QString)));
    connect(remoteApp,SIGNAL(sendFatalMessage(QString)), this,SLOT(fatalMessage(QString)));

    connect(localApp,SIGNAL(setupForRun(QString &,QString &)), this, SLOT(setUpForApplicationRun(QString &,QString &)));
    connect(this,SIGNAL(setUpForApplicationRunDone(QString&, QString &)), theRunWidget, SLOT(setupForRunApplicationDone(QString&, QString &)));
    connect(localApp,SIGNAL(processResults(QString, QString, QString)), this, SLOT(processResults(QString, QString, QString)));

    connect(remoteApp,SIGNAL(setupForRun(QString &,QString &)), this, SLOT(setUpForApplicationRun(QString &,QString &)));

    connect(theJobManager,SIGNAL(processResults(QString , QString, QString)), this, SLOT(processResults(QString, QString, QString)));
    connect(theJobManager,SIGNAL(loadFile(QString)), this, SLOT(loadFile(QString)));

    connect(remoteApp,SIGNAL(successfullJobStart()), theRunWidget, SLOT(hide()));


    //
    // create layout to hold component selectio
    //


    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    this->setLayout(horizontalLayout);
    this->setContentsMargins(0,5,0,5);
    horizontalLayout->setMargin(0);

    //
    // create the component selection & add the components to it
    //

    theComponentSelection = new SimCenterComponentSelection();
    horizontalLayout->addWidget(theComponentSelection);

    theComponentSelection->setMaxWidth(120);

    theAssetsWidget->setObjectName("Assets");
    theHazardsWidget->setObjectName("Hazards");
    theResults->setObjectName("Results");
    theVisualizationWidget->setObjectName("Visualization");

    theComponentSelection->addComponent(QString("Assets"), theAssetsWidget);
    theComponentSelection->addComponent(QString("Hazards"), theHazardsWidget);
    //    theComponentSelection->addComponent(QString("UQ"),  theUQ_Selection);
    //    theComponentSelection->addComponent(QString("GI"),  theGI);
    //    theComponentSelection->addComponent(QString("SIM"), theSIM);
    //    theComponentSelection->addComponent(QString("EVT"), theEventSelection);
    //    theComponentSelection->addComponent(QString("FEM"), theAnalysisSelection);
    //    theComponentSelection->addComponent(QString("EDP"), theEDP_Selection);
    //    theComponentSelection->addComponent(QString("RV"),  theRVs);
    theComponentSelection->addComponent(QString("Results"), theResults);
    theComponentSelection->addComponent(QString("Visualization"), theVisualizationWidget);

//    theComponentSelection->displayComponent("Visualization");
    theComponentSelection->displayComponent("Hazards");

    // access a web page which will increment the usage count for this tool
    manager = new QNetworkAccessManager(this);

    connect(manager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));

    manager->get(QNetworkRequest(QUrl("http://opensees.berkeley.edu/OpenSees/developer/eeuq/use.php")));

}

WorkflowAppRDT::~WorkflowAppRDT()
{

}

void WorkflowAppRDT::replyFinished(QNetworkReply *pReply)
{
    return;
}

HazardsWidget *WorkflowAppRDT::getTheHazardsWidget() const
{
    return theHazardsWidget;
}

AssetsWidget *WorkflowAppRDT::getTheAssetsWidget() const
{
    return theAssetsWidget;
}

VisualizationWidget *WorkflowAppRDT::getTheVisualizationWidget() const
{
    return theVisualizationWidget;
}


void WorkflowAppRDT::setActiveWidget(SimCenterAppWidget* widget)
{
    auto widgetName = widget->objectName();

    this->theComponentSelection->displayComponent(widgetName);
}


bool WorkflowAppRDT::outputToJSON(QJsonObject &jsonObjectTop) {

    //
    // get each of the main widgets to output themselves
    //

    QJsonObject apps;

    QJsonObject jsonObjGenInfo;
    //    theGI->outputToJSON(jsonObjGenInfo);
    jsonObjectTop["GeneralInformation"] = jsonObjGenInfo;

    QJsonObject jsonObjStructural;
    //    theSIM->outputToJSON(jsonObjStructural);

    jsonObjectTop["StructuralInformation"] = jsonObjStructural;
    QJsonObject appsSIM;
    //    theSIM->outputAppDataToJSON(appsSIM);

    apps["Modeling"]=appsSIM;

    //    theRVs->outputToJSON(jsonObjectTop);

    QJsonObject jsonObjectEDP;
    //    theEDP_Selection->outputToJSON(jsonObjectEDP);
    jsonObjectTop["EDP"] = jsonObjectEDP;

    QJsonObject appsEDP;
    //    theEDP_Selection->outputAppDataToJSON(appsEDP);
    apps["EDP"]=appsEDP;

    /*
    QJsonObject jsonObjectUQ;
    theUQ_Selection->outputToJSON(jsonObjectUQ);
    jsonObjectTop["UQ_Method"] = jsonObjectUQ;
    */


    //    theUQ_Selection->outputAppDataToJSON(apps);
    //    theUQ_Selection->outputToJSON(jsonObjectTop);

    //    theAnalysisSelection->outputAppDataToJSON(apps);
    //    theAnalysisSelection->outputToJSON(jsonObjectTop);

    // NOTE: Events treated differently, due to array nature of objects
    //    theEventSelection->outputToJSON(jsonObjectTop);
    //    theEventSelection->outputAppDataToJSON(apps);

    theRunWidget->outputToJSON(jsonObjectTop);

    jsonObjectTop["Applications"]=apps;

    //theRunLocalWidget->outputToJSON(jsonObjectTop);

    return true;
}


void WorkflowAppRDT::processResults(QString dakotaOut, QString dakotaTab, QString inputFile){


    //
    // get results widget fr currently selected UQ option
    //

    theResults=theUQ_Selection->getResults();
    if (theResults == NULL) {
        this->errorMessage("FATAL - UQ option selected not returning results widget");
        return;
    }

    //
    // connect signals for results widget
    //

    connect(theResults,SIGNAL(sendStatusMessage(QString)), this,SLOT(statusMessage(QString)));
    connect(theResults,SIGNAL(sendErrorMessage(QString)), this,SLOT(errorMessage(QString)));

    //
    // swap current results with existing one in selection & disconnect signals
    //

    QWidget *oldResults = theComponentSelection->swapComponent(QString("RES"), theResults);
    if (oldResults != NULL) {
        disconnect(oldResults,SIGNAL(sendErrorMessage(QString)), this,SLOT(errorMessage(QString)));
        disconnect(oldResults,SIGNAL(sendFatalMessage(QString)), this,SLOT(fatalMessage(QString)));
        delete oldResults;
    }

    //
    // proess results
    //

    theResults->processResults(dakotaOut, dakotaTab);
    theRunWidget->hide();
    theComponentSelection->displayComponent("RES");
}


void WorkflowAppRDT::clear(void)
{

}


bool WorkflowAppRDT::inputFromJSON(QJsonObject &jsonObject)
{
    //
    // get each of the main widgets to input themselves
    //

    if (jsonObject.contains("GeneralInformation")) {
        QJsonObject jsonObjGeneralInformation = jsonObject["GeneralInformation"].toObject();
        //        if (theGI->inputFromJSON(jsonObjGeneralInformation) == false) {
        //            emit errorMessage("EE_UQ: failed to read GeneralInformation");
        //        }
    } else {
        emit errorMessage("EE_UQ: failed to find GeneralInformation");
        return false;
    }

    if (jsonObject.contains("Applications")) {

        QJsonObject theApplicationObject = jsonObject["Applications"].toObject();

        if (theApplicationObject.contains("Modeling")) {
            QJsonObject theObject = theApplicationObject["Modeling"].toObject();
            //            if (theSIM->inputAppDataFromJSON(theObject) == false) {
            //                emit errorMessage("EE_UQ: failed to read Modeling Application");
            //            }
        } else {
            emit errorMessage("EE_UQ: failed to find Modeling Application");
            return false;
        }

        // note: Events is different because the object is an Array
        if (theApplicationObject.contains("Events")) {
            //  QJsonObject theObject = theApplicationObject["Events"].toObject(); it is null object, actually an array
            //            if (theEventSelection->inputAppDataFromJSON(theApplicationObject) == false) {
            //                emit errorMessage("EE_UQ: failed to read Event Application");
            //            }

        } else {
            emit errorMessage("EE_UQ: failed to find Event Application");
            return false;
        }

        if (theUQ_Selection->inputAppDataFromJSON(theApplicationObject) == false)
            emit errorMessage("EE_UQ: failed to read UQ application");

        //        if (theAnalysisSelection->inputAppDataFromJSON(theApplicationObject) == false)
        //            emit errorMessage("EE_UQ: failed to read FEM application");

        if (theApplicationObject.contains("EDP")) {
            QJsonObject theObject = theApplicationObject["EDP"].toObject();
            //            if (theEDP_Selection->inputAppDataFromJSON(theObject) == false) {
            //                emit errorMessage("EE_UQ: failed to read EDP application");
            //            }
        } else {
            emit errorMessage("EE_UQ: failed to find EDP application");
            return false;
        }

    } else
        return false;

    /*
    ** Note to me - RVs and Events treated differently as both use arrays .. rethink API!
    */

    //    theEventSelection->inputFromJSON(jsonObject);
    //    theRVs->inputFromJSON(jsonObject);
    theRunWidget->inputFromJSON(jsonObject);

    if (jsonObject.contains("StructuralInformation")) {
        QJsonObject jsonObjStructuralInformation = jsonObject["StructuralInformation"].toObject();
        //        if (theSIM->inputFromJSON(jsonObjStructuralInformation) == false) {
        //            emit errorMessage("EE_UQ: failed to read StructuralInformation");
        //        }
    } else {
        emit errorMessage("EE_UQ: failed to find StructuralInformation");
        return false;
    }

    if (jsonObject.contains("EDP")) {
        QJsonObject edpObj = jsonObject["EDP"].toObject();
        //        if (theEDP_Selection->inputFromJSON(edpObj) == false)
        //            emit errorMessage("EE_UQ: failed to read EDP data");
    } else {
        emit errorMessage("EE_UQ: failed to find EDP data");
        return false;
    }


    if (theUQ_Selection->inputFromJSON(jsonObject) == false)
        emit errorMessage("EE_UQ: failed to read UQ Method data");

    //    if (theAnalysisSelection->inputFromJSON(jsonObject) == false)
    //        emit errorMessage("EE_UQ: failed to read FEM Method data");

    return true;
}


void WorkflowAppRDT::onRunButtonClicked() {
    theRunWidget->hide();
    theRunWidget->setMinimumWidth(this->width()*0.5);
    theRunWidget->showLocalApplication();
    GoogleAnalytics::ReportLocalRun();
}


void WorkflowAppRDT::onRemoteRunButtonClicked(){
    emit errorMessage("");

    bool loggedIn = theRemoteService->isLoggedIn();

    if (loggedIn == true) {
        theRunWidget->hide();
        theRunWidget->setMinimumWidth(this->width()*0.5);
        theRunWidget->showRemoteApplication();

    } else {
        errorMessage("ERROR - You Need to Login");
    }

    GoogleAnalytics::ReportDesignSafeRun();
}


void WorkflowAppRDT::onRemoteGetButtonClicked(){

    emit errorMessage("");

    bool loggedIn = theRemoteService->isLoggedIn();

    if (loggedIn == true) {

        theJobManager->hide();
        theJobManager->updateJobTable("");
        theJobManager->show();

    } else {
        errorMessage("ERROR - You Need to Login");
    }
}


void WorkflowAppRDT::onExitButtonClicked(){

}


void WorkflowAppRDT::setUpForApplicationRun(QString &workingDir, QString &subDir) {

    errorMessage("");

    //
    // create temporary directory in working dir
    // and copy all files needed to this directory by invoking copyFiles() on app widgets
    //

    // designsafe will need a unique name
    /* *********************************************
    will let ParallelApplication rename dir
    QUuid uniqueName = QUuid::createUuid();
    QString strUnique = uniqueName.toString();
    strUnique = strUnique.mid(1,36);
    QString tmpDirName = QString("tmp.SimCenter") + strUnique;
    *********************************************** */

    QString tmpDirName = QString("tmp.SimCenter");
    qDebug() << "TMP_DIR: " << tmpDirName;
    QDir workDir(workingDir);

    QString tmpDirectory = workDir.absoluteFilePath(tmpDirName);
    QDir destinationDirectory(tmpDirectory);

    if(destinationDirectory.exists()) {
        destinationDirectory.removeRecursively();
    } else
        destinationDirectory.mkpath(tmpDirectory);

    QString templateDirectory  = destinationDirectory.absoluteFilePath(subDir);
    destinationDirectory.mkpath(templateDirectory);

    // copyPath(path, tmpDirectory, false);
    //    theSIM->copyFiles(templateDirectory);
    //    theEventSelection->copyFiles(templateDirectory);
    //    theAnalysisSelection->copyFiles(templateDirectory);
    theUQ_Selection->copyFiles(templateDirectory);
    //    theEDP_Selection->copyFiles(templateDirectory);

    //
    // in new templatedir dir save the UI data into dakota.json file (same result as using saveAs)
    // NOTE: we append object workingDir to this which points to template dir
    //

    QString inputFile = templateDirectory + QDir::separator() + tr("dakota.json");

    QFile file(inputFile);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        //errorMessage();
        return;
    }
    QJsonObject json;
    this->outputToJSON(json);

    json["runDir"]=tmpDirectory;
    json["WorkflowType"]="Building Simulation";


    QJsonDocument doc(json);
    file.write(doc.toJson());
    file.close();


    statusMessage("SetUp Done .. Now starting application");

    emit setUpForApplicationRunDone(tmpDirectory, inputFile);
}


void WorkflowAppRDT::loadFile(const QString fileName){

    //
    // open file
    //

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        emit errorMessage(QString("Could Not Open File: ") + fileName);
        return;
    }

    //
    // place contents of file into json object
    //

    QString val;
    val=file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8());
    QJsonObject jsonObj = doc.object();

    // close file
    file.close();

    //
    // clear current and input from new JSON
    //

    this->clear();
    this->inputFromJSON(jsonObj);
}


int WorkflowAppRDT::getMaxNumParallelTasks() {
    return theUQ_Selection->getNumParallelTasks();
}
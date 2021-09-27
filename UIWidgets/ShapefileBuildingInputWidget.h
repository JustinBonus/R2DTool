#ifndef ShapefileBuildingInputWidget_H
#define ShapefileBuildingInputWidget_H
/* *****************************************************************************
Copyright (c) 2016-2021, The Regents of the University of California (Regents).
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
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
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

#include "SimCenterAppWidget.h"
#include "ComponentDatabase.h"

#include <set>

#include <QString>
#include <QObject>

class AssetInputDelegate;
class ComponentTableView;
class VisualizationWidget;

class QgsFeature;
class QGISVisualizationWidget;
class QgsVectorLayer;
class QgsGeometry;

class QGroupBox;
class QLineEdit;
class QTableWidget;
class QLabel;

class ShapefileBuildingInputWidget : public  SimCenterAppWidget
{
    Q_OBJECT

public:
    explicit ShapefileBuildingInputWidget(QWidget *parent, VisualizationWidget* visWidget, QString componentType, QString appType = QString());
    virtual ~ShapefileBuildingInputWidget();

    virtual QgsFeature*  addFeatureToSelectedLayer(QMap<QString, QVariant>& featureAttributes, QgsGeometry& geom);
    virtual int removeFeatureFromSelectedLayer(QgsFeature* feat);
    virtual QgsVectorLayer* getSelectedFeatureLayer(void);

    ComponentTableView *getTableWidget() const;

    // Set the filter string and select the components
    void setFilterString(const QString& filter);
    QString getFilterString(void);

    void insertSelectedComponent(const int ComponentID);

    int numberComponentsSelected(void);

    void updateComponentAttribute(const int ID, const QString& attribute, const QVariant& value);
    void updateSelectedComponentAttribute(const QString& uid, const QString& attribute, const QVariant& value);

    // Set custom labels in widget
    void setComponentType(const QString &value);
    void setLabel1(const QString &value);
    void setLabel2(const QString &value);
    void setLabel3(const QString &value);
    void setGroupBoxText(const QString &value);

    void loadFileFromPath(QString& path);

    bool outputAppDataToJSON(QJsonObject &jsonObject);
    bool inputAppDataFromJSON(QJsonObject &jsonObject);
    bool outputToJSON(QJsonObject &rvObject);
    bool inputFromJSON(QJsonObject &rvObject);
    bool copyFiles(QString &destName);

    QString getPathToComponentFile(void) const;

    virtual void clear(void);

    QStringList getTableHorizontalHeadings();

    // Selects all of the components for analysis
    void selectAllComponents(void);

signals:
    void headingValuesChanged(QStringList);

public slots:
    void handleComponentSelection(void);
    void handleCellChanged(const int row, const int col);

private slots:
    void selectComponents(void);
    void loadComponentData(void);
    void chooseComponentInfoFileDialog(void);
    void clearComponentSelection(void);
    void clearLayerSelectedForAnalysis(void);

protected:

    QGISVisualizationWidget* theVisualizationWidget;

    ComponentTableView* componentTableWidget;

    ComponentDatabase* theComponentDb;

    // Returns a vector of sorted items that are unique
    template <typename T>
    void uniqueVec(std::vector<T>& vec)
    {
        std::sort(vec.begin(), vec.end());

        // Using std::unique to get the unique items in the vector
        auto ip = std::unique(vec.begin(), vec.end());

        // Resizing the vector so as to remove the terms that became undefined after the unique operation
        vec.resize(std::distance(vec.begin(), ip));
    }

private:
    QString pathToComponentInputFile;
    QLineEdit* componentFileLineEdit;
    AssetInputDelegate* selectComponentsLineEdit;
    QLabel* componentInfoText;
    QGroupBox* componentGroupBox;

    QString appType;
    QString componentType;
    QString label1;
    QString label2;
    QString label3;

    QStringList tableHorizontalHeadings;

    void createComponentsBox(void);

    // Map to store the selected features according to their UID
    QMap<QgsFeatureId, QgsFeature> selectedFeaturesForAnalysis;

};

#endif // ShapefileBuildingInputWidget_H
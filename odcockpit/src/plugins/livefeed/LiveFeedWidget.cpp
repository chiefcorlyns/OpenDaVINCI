/**
 * cockpit - Visualization environment
 * Copyright (C) 2012 - 2016 Christian Berger
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QtCore>
#include <QtGui>

#include <cstring>
#include <iostream>
#include <vector>

#include "opendavinci/odcore/opendavinci.h"
#include "opendavinci/odcore/base/Lock.h"
#include "opendavinci/odcore/data/Container.h"
#include "opendavinci/odcore/data/TimeStamp.h"
#include "opendavinci/odcore/reflection/Message.h"
#include "opendavinci/odcore/strings/StringToolbox.h"

#include "plugins/livefeed/LiveFeedWidget.h"
#include "plugins/livefeed/MessageToTupleVisitor.h"

namespace cockpit { namespace plugins { class PlugIn; } }

namespace cockpit {

    namespace plugins {

        namespace livefeed {

            using namespace std;
            using namespace odcore::base;
            using namespace odcore::data;
            using namespace odcore::reflection;

            LiveFeedWidget::LiveFeedWidget(const odcore::base::KeyValueConfiguration &kvc, const PlugIn &/*plugIn*/, QWidget *prnt) :
                QWidget(prnt),
                m_messageResolver(),
                m_dataViewMutex(),
                m_lastContainerSampleTime(),
                m_dataView(),
                m_dataToType(),
                m_containerTypeToName(),
                m_containerTypeResolvingMutex(),
                m_containerTypeResolving() {
                // Set size.
                setMinimumSize(400, 200);

                // Layout manager.
                QGridLayout* mainBox = new QGridLayout(this);

                m_lastContainerSampleTime = new QLabel("Sample time: ", this);
                mainBox->addWidget(m_lastContainerSampleTime, 0, 0);

                //ListView and header construction
                m_dataView = new QTreeWidget(this);
                m_dataView->setColumnCount(2);
                QStringList headerLabel;
                headerLabel << tr("Message") << tr("Value");
                m_dataView->setColumnWidth(0, 200);
                m_dataView->setColumnWidth(1, 200);
                m_dataView->setHeaderLabels(headerLabel);

                connect(m_dataView, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(treeItemChanged(QTreeWidgetItem*, int)));

                //add to Layout
                mainBox->addWidget(m_dataView, 1, 0);

                // Set layout manager.
                setLayout(mainBox);

                // Try to load shared libaries.
                const string SEARCH_PATH = kvc.getValue<string>("odcockpit.directoriesForSharedLibaries");
                cout << "[odcockpit/livefeed] Trying to find libodvd*.so files in: " << SEARCH_PATH << endl;

                const vector<string> paths = odcore::strings::StringToolbox::split(SEARCH_PATH, ',');
                m_messageResolver = unique_ptr<MessageResolver>(new MessageResolver(paths, "libodvd", ".so"));
            }

            LiveFeedWidget::~LiveFeedWidget() {}

            void LiveFeedWidget::nextContainer(Container &container) {
                Lock l(m_dataViewMutex);
                if (container.getDataType() == odcore::data::player::PlayerStatus::ID()) {
                    odcore::data::player::PlayerStatus ps = container.getData<odcore::data::player::PlayerStatus>();
                    if (ps.getStatus() == odcore::data::player::PlayerStatus::NEW_FILE_LOADED) {
                        // Clear current entries.
                        for (auto it = m_dataToType.begin(); it != m_dataToType.end(); it++) {
                            QTreeWidgetItem *entry = it->second;
                            entry->takeChildren();
                        }
                        m_dataToType.clear();
                        m_dataView->clear();

                        m_lastContainerSampleTime->setText("Sample time: ");
                    }
                }
                else {
                    transformContainerToTree(container);
                }
            }

            void LiveFeedWidget::treeItemChanged(QTreeWidgetItem *twi, int col) {
                if ( (NULL != twi) && (0 == col) ) {
                    Lock l(m_containerTypeResolvingMutex);
                    m_containerTypeResolving[twi->text(col).toStdString()] = (twi->checkState(col) == Qt::Checked);
                }
            }

            void LiveFeedWidget::transformContainerToTree(Container &container) {
                // Map attributes from message to the entries.
                bool successfullyMapped = false;
                Message msg;

                if (container.getDataType() != odcore::data::dmcp::ModuleStatistics::ID()) {
                    stringstream sstr_sampleTime;
                    sstr_sampleTime << "Sample time: " << container.getSampleTimeStamp().getYYYYMMDD_HHMMSSms();
                    const string str_sampleTime = sstr_sampleTime.str();
                    m_lastContainerSampleTime->setText(str_sampleTime.c_str());
                }

                if (0 == (m_containerTypeToName.count(container.getDataType()))) {
                    msg = m_messageResolver->resolve(container, successfullyMapped);
                    if (successfullyMapped) {
                        m_containerTypeToName[container.getDataType()] = msg.getLongName();

                        Lock l(m_containerTypeResolvingMutex);
                        stringstream sstr;
                        sstr << m_containerTypeToName[container.getDataType()] << "/" << container.getSenderStamp();
                        m_containerTypeResolving[sstr.str()] = false;
                    }
                }
                else {
                    vector<pair<string, string> > entries;
                    {
                        stringstream sstr;
                        sstr << container.getDataType();
                        const string str = sstr.str();
                        entries.push_back(make_pair("type", str));
                    }
                    {
                        stringstream sstr;
                        sstr << container.getSenderStamp();
                        const string str = sstr.str();
                        entries.push_back(make_pair("sender stamp", str));
                    }
                    entries.push_back(make_pair("sent", container.getSentTimeStamp().getYYYYMMDD_HHMMSSms()));
                    entries.push_back(make_pair("received", container.getReceivedTimeStamp().getYYYYMMDD_HHMMSSms()));
                    entries.push_back(make_pair("sample time", container.getSampleTimeStamp().getYYYYMMDD_HHMMSSms()));

                    // Create new Header if needed.
                    stringstream sstr_entryName;
                    sstr_entryName << m_containerTypeToName[container.getDataType()] << "/" << container.getSenderStamp();
                    const string entryName = sstr_entryName.str();

                    {
                        Lock l(m_containerTypeResolvingMutex);
                        if (m_containerTypeResolving[entryName]) {
                            msg = m_messageResolver->resolve(container, successfullyMapped);
                            if (successfullyMapped) {
                                MessageToTupleVisitor mttv(entries);
                                msg.accept(mttv);
                            }
                        }
                    }

                    if (m_dataToType.find(entryName) == m_dataToType.end()) {
                        QTreeWidgetItem *newHeader = new QTreeWidgetItem(m_dataView);
                        newHeader->setText(0, entryName.c_str());
                        m_dataToType[entryName] = newHeader;

                        newHeader->setFlags(newHeader->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
                        newHeader->setCheckState(0, Qt::Unchecked);
                    }

                    QTreeWidgetItem *entry = m_dataToType[entryName];
                    if (static_cast<uint32_t>(entry->childCount()) != entries.size()) {
                        entry->takeChildren();

                        for (uint32_t i = 0; i < entries.size(); i++) {
                            QTreeWidgetItem *sent = new QTreeWidgetItem();
                            entry->insertChild(i, sent);
                        }
                    }

                    // Map tuples of <string, string> to the tree.
                    for (uint32_t i = 0; i < entries.size(); i++) {
                        QTreeWidgetItem *child = entry->child(i);
                        child->setText(0, entries.at(i).first.c_str());
                        child->setText(1, entries.at(i).second.c_str());
                    }
                }
            }
        }
    }
}


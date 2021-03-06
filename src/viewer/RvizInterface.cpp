/**
 * Copyright 2017
 * 
 * This file is part of On-line POMDP Planning Toolkit (OPPT).
 * OPPT is free software: you can redistribute it and/or modify it under the terms of the 
 * GNU General Public License published by the Free Software Foundation, 
 * either version 2 of the License, or (at your option) any later version.
 * 
 * OPPT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with OPPT. 
 * If not, see http://www.gnu.org/licenses/.
 */
#include "RvizInterface.hpp"
#include <rviz/display_group.h>
#include <rviz/default_plugin/axes_display.h>
#include <functional>
#include <QTextEdit>

using std::cout;
using std::endl;

namespace oppt
{

DisplayManager::DisplayManager(rviz::RenderPanel* renderPanel):
    manager_(new rviz::VisualizationManager(renderPanel))
{
    renderPanel->initialize(manager_->getSceneManager(), manager_.get());
    manager_->initialize();
    manager_->startUpdate();
    ros::NodeHandle node;
    displayAddedPub_ = node.advertise<std_msgs::Bool>("displayAddedTopic", 1, false);
}

DisplayManager::~DisplayManager()
{

}

bool DisplayManager::removeDisplay(std::string name)
{
    int numDisplays = manager_->getRootDisplayGroup()->numDisplays();
    rviz::Display* display = nullptr;
    for (int i = 0; i < numDisplays; i++) {
        display = manager_->getRootDisplayGroup()->getDisplayAt(i);
        if (display->getName().toStdString() == name) {
            display = manager_->getRootDisplayGroup()->takeDisplay(display);
            if (display) {
                delete display;
                break;
            }
        }
    }
}


rviz::Display* DisplayManager::addDisplay(AddDisplayCommand* addDisplayCommand)
{
    manager_->stopUpdate();
    const QString displayTypeQ = QString::fromStdString(addDisplayCommand->displayType);
    const QString nameQ = QString::fromStdString(addDisplayCommand->name);
    rviz::Display* display =
        manager_->createDisplay(displayTypeQ, nameQ, addDisplayCommand->enabled);
    if (addDisplayCommand->topic != "" && addDisplayCommand->topicType != "") {
        const QString topicString = QString::fromStdString(addDisplayCommand->topic);
        const QString topicType = QString::fromStdString(addDisplayCommand->topicType);
        display->setTopic(topicString, topicType);
    }

    /**if (addDisplayCommand->displayType == "rviz/Axes") {
        const QString frameStr = QString::fromStdString(addDisplayCommand->frame);
        display->setFixedFrame(frameStr);
    static_cast<rviz::AxesDisplay *>(display)->set(10.0, 0.3);
    }*/

    if (addDisplayCommand->displayType == "rviz/Axes") {
        const QString frameStr = QString::fromStdString(addDisplayCommand->frame);
        display->setFixedFrame(frameStr);
        int i = 0;
        while (true) {
            auto property = display->childAt(i);
            if (!property)
                break;
            if (property->getNameStd() == "Length") {
                QVariant lengthV(0.75);
                property->setValue(lengthV);
            }

            if (property->getNameStd() == "Radius") {
                QVariant radiusV(0.015);
                property->setValue(radiusV);
            }

            i++;
        }
    }

    display->setEnabled(addDisplayCommand->enabled);
    //displays_[addDisplayCommand->name] = display;
    manager_->startUpdate();
    std_msgs::Bool msg;
    msg.data = true;    
    displayAddedPub_.publish(msg);
    ros::spinOnce();    
    return display;
}

rviz::VisualizationManager* DisplayManager::getManager() const
{
    return manager_.get();
}


RvizWidget::RvizWidget(QWidget* parent):
    QWidget(parent),
    mainLayout_(new QGridLayout())
{

}

bool RvizWidget::init(rviz::RenderPanel* renderPanel)
{
    //main_layout->addLayout(controls_layout);

    //QGridLayout* controls_layout = new QGridLayout();
    QTextEdit* textEdit = new QTextEdit();
    //controls_layout->addWidget( textEdit, 0, 0 );
    mainLayout_->addWidget(renderPanel, 0, 0, -1, 1);
    //mainLayout_->addWidget(textEdit, 1, 1, 1, 1);

    // Set the top-level layout for this MyViz widget.
    setLayout(mainLayout_.get());
}


RvizInterface::RvizInterface()
{
    //killRos();
    int argc = 0;
    char** argv;
    if (!initializeRos())
	return;
    ros::init(argc, argv, "myviz", ros::init_options::AnonymousName);
    ros::NodeHandle node;
    std::string topic = "the_markers";
    vizPub_ = node.advertise<visualization_msgs::MarkerArray>(topic, 0);    

    rosInitialized_ = true;
    dQueue_ = std::make_shared<CommandQueue>();
    viewerThread_ = std::unique_ptr<std::thread>(new std::thread(&RvizInterface::run, this, dQueue_));
    //viewerThread_ = new std::thread(&RvizInterface::run, this, dQueue_);

    DisplayCommand* comm = new AddDisplayCommand();
    static_cast<AddDisplayCommand*>(comm)->name = "MarkerArray";
    static_cast<AddDisplayCommand*>(comm)->displayType = "rviz/MarkerArray";
    static_cast<AddDisplayCommand*>(comm)->topic = topic;
    static_cast<AddDisplayCommand*>(comm)->topicType = "visualization_msgs/MarkerArray";
    addToQueue(comm);

    /**DisplayCommand* comm = new AddDisplayCommand();
    static_cast<AddDisplayCommand*>(comm)->name = "BaseFramePose";
    static_cast<AddDisplayCommand*>(comm)->displayType = "rviz/MarkerArray";
    static_cast<AddDisplayCommand*>(comm)->topic = topic;
    static_cast<AddDisplayCommand*>(comm)->topicType = "visualization_msgs/MarkerArray";
    addToQueue(comm);*/

    /**AddDisplayCommand* comm2 = new AddDisplayCommand();
    comm2->name = "adjGrid";
    comm2->displayType = "rviz/Grid";
    addToQueue(comm2);*/
    /**AddDisplayCommand* comm2 = new AddDisplayCommand();
    comm2->name = "axes";
    comm2->displayType = "rviz/Axes";
    addToQueue(comm2);*/

    viewerSetup_ = true;
    initSubscriber();
}

void RvizInterface::init()
{

}

void RvizInterface::initSubscriber()
{
    ros::NodeHandle n;
    ros::Subscriber sub = n.subscribe("viewerTopic",
                                      1000,
                                      &RvizInterface::receiveCommands,
                                      this);
    ros::Subscriber sub2 = n.subscribe("setFixedFrameTopic",
                                       1000,
                                       &RvizInterface::receiveBaseFrameCommand,
                                       this);

    ros::Subscriber sub3 = n.subscribe("resetFrameTopic",
                                       1000,
                                       &RvizInterface::reset,
                                       this);

    ros::Subscriber sub4 = n.subscribe("grabScreenTopic",
                                       1000,
                                       &RvizInterface::receiveGrabScreenCommand,
                                       this);

    ros::Subscriber sub5 = n.subscribe("showWorldFrameTopic",
                                       1000,
                                       &RvizInterface::receiveShowWorldFrameCommand,
                                       this);

    cout << "Spinnning" << endl;
    ros::spin();
}

void RvizInterface::addToQueue(DisplayCommand* command)
{
    mtx_.lock();
    dQueue_->push(command);
    mtx_.unlock();
}

void RvizInterface::processQueue(CommandQueuePtr& dQueue)
{
    while (dQueue->size() > 0) {
        mtx_.lock();
        DisplayCommand* comm = dQueue->front();
        mtx_.unlock();
        if (comm->typeString == "addDisplay") {
            AddDisplayCommand* addCommand = static_cast<AddDisplayCommand*>(comm);
            displayManager_->addDisplay(addCommand);
        } else if (comm->typeString == "removeDisplay") {
            std::string displayName = static_cast<RemoveDisplayCommand*>(comm)->name;
            displayManager_->removeDisplay(displayName);
        } else if (comm->typeString == "setFixedFrame") {
            const QString fixedFrameStr =
                QString::fromStdString(static_cast<SetFixedFrameCommand*>(comm)->fixedFrame);
            displayManager_->getManager()->setFixedFrame(fixedFrameStr);
        } /**else if (comm->typeString == "takeScreenshot") {
            std::string filename = static_cast<TakeScreenshotCommand*>(comm)->filename;
            takeScreenshotImpl(filename);
        }*/

        delete comm;
        mtx_.lock();
        dQueue->pop();
        mtx_.unlock();
    }
}

void RvizInterface::receiveCommands(const visualization_msgs::MarkerArrayConstPtr& msg)
{    
    vizPub_.publish(*(msg.get()));
}

void RvizInterface::receiveBaseFrameCommand(const std_msgs::StringConstPtr& msg)
{
    DisplayCommand* comm = new SetFixedFrameCommand();
    static_cast<SetFixedFrameCommand*>(comm)->fixedFrame = msg->data;
    addToQueue(comm);
    fixedFrame_ = msg->data;
}

void RvizInterface::receiveGrabScreenCommand(const std_msgs::StringConstPtr& msg)
{
    QPixmap screenshot = QPixmap::grabWindow(renderPanel_->winId());
    const QString filename = QString::fromStdString(msg->data);
    bool saved = screenshot.save(filename);
    if (saved) {
        cout << "Screenshot saved: " << msg->data << endl;
        return;
    }

    cout << "Couldn't save screenshot to '" << msg->data << "'" << endl;

}

void RvizInterface::receiveShowWorldFrameCommand(const std_msgs::BoolConstPtr& msg)
{    
    if (!fixedFrame_.empty() && msg->data == true) {	
        DisplayCommand* comm3 = new AddDisplayCommand();
        static_cast<AddDisplayCommand*>(comm3)->name = "Axes";
        static_cast<AddDisplayCommand*>(comm3)->displayType = "rviz/Axes";
        static_cast<AddDisplayCommand*>(comm3)->frame = fixedFrame_;
        addToQueue(comm3);
    } else {
        DisplayCommand* comm2 = new RemoveDisplayCommand();
        static_cast<RemoveDisplayCommand*>(comm2)->name = "Axes";
        addToQueue(comm2);
    }
}

bool RvizInterface::initializeRos()
{
    int argc = 0;
    char** argv;
    char const* tmp = std::getenv("ROS_MASTER_URI");
    if (!tmp) {
	cout << "ERROR: ROS_MASTER_URI is not defined in the environment." << endl;
	return false;
    }
    //roscoreThread_ = new std::thread(&RvizInterface::startRoscore, this);
    //startRoscore();
    roscoreThread_ = std::unique_ptr<std::thread>(new std::thread(&RvizInterface::startRoscore, this));
    return true;
}

bool RvizInterface::startRoscore()
{
    if (!ros::master::check()) {
        int out = system("roscore");
    }
}

void RvizInterface::run(CommandQueuePtr dQueue)
{
    int argc = 0;
    char** argv;
    QApplication app2(argc, argv);
    while (!ros::master::check()) {
    }

    renderPanel_ = std::unique_ptr<rviz::RenderPanel>(new rviz::RenderPanel());
    rvizWidget_ = std::unique_ptr<RvizWidget>(new RvizWidget());
    rvizWidget_->init(renderPanel_.get());
    displayManager_ = std::unique_ptr<DisplayManager>(new DisplayManager(renderPanel_.get()));
    rvizWidget_->show();
    viewerLoaded_ = true;
    while (!destroy_) {
        processQueue(dQueue);
        app2.sendPostedEvents();
        app2.processEvents();
        usleep(0.05e5);
    }
}

void RvizInterface::reset(const std_msgs::BoolConstPtr& b)
{
    DisplayCommand* comm2 = new RemoveDisplayCommand();
    static_cast<RemoveDisplayCommand*>(comm2)->name = "MarkerArray";
    addToQueue(comm2);
    DisplayCommand* comm = new AddDisplayCommand();
    static_cast<AddDisplayCommand*>(comm)->name = "MarkerArray";
    static_cast<AddDisplayCommand*>(comm)->displayType = "rviz/MarkerArray";
    static_cast<AddDisplayCommand*>(comm)->topic = "the_markers";
    static_cast<AddDisplayCommand*>(comm)->topicType = "visualization_msgs/MarkerArray";
    addToQueue(comm);    
}

}

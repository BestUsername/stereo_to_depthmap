#include <QFileDialog>
#include <QProgressDialog>
#include <QMessageBox>
#include <iostream>

#include "opencv2/imgproc/imgproc.hpp"

#include "processor.h"
#include "qtopencvdepthmap.h"
#include "ui_qtopencvdepthmap.h"

/**
 * Constructor for the main GUI widget.
 * @param args The command-line arguments object parsed by argp.
 * @param parent Standard QWidget parent.
 */
QtOpenCVDepthmap::QtOpenCVDepthmap(Arguments& args, QWidget *parent) :
    QMainWindow(parent),
    arguments(args),
    ui(new Ui::QtOpenCVDepthmap)
{
    first_load = true;
    args_to_mapper();
    ui->setupUi(this);

    set_active(false);

    std::string input = arguments.get_value<std::string>(Arguments::INPUT_FILENAME);

    if (!input.empty()) {
        open_filename(input);
    }
}

/**
 * Destructor cleans up the UI.
 */
QtOpenCVDepthmap::~QtOpenCVDepthmap()
{
    delete ui;
}

/**
 * Set whether the application is "active". Active meaning that a valid input is loaded and the configuration controls should be enabled.
 * @param isActive If the application should be "active" or not.
 */
void QtOpenCVDepthmap::set_active(bool isActive) {
    //set disparity inputs enabled/disabled
    ui->input_minDisparity->setEnabled(isActive);
    ui->input_numDisparities->setEnabled(isActive);
    ui->input_SADWindowSize->setEnabled(isActive);
    ui->input_P1->setEnabled(isActive);
    ui->input_P2->setEnabled(isActive);
    ui->input_disp12MAxDiff->setEnabled(isActive);
    ui->input_preFilterCap->setEnabled(isActive);
    ui->input_uniqunessRatio->setEnabled(isActive);
    ui->input_speckleWindowSize->setEnabled(isActive);
    ui->input_speckleRange->setEnabled(isActive);
    ui->input_fullDP->setEnabled(isActive);
    //set position inputs enabled/disabled
    ui->horizontalSlider->setEnabled(isActive);
    ui->spinBox_clip_start->setEnabled(isActive);
    ui->spinBox_current_frame->setEnabled(isActive);
    ui->spinBox_clip_end->setEnabled(isActive);
    //check_clip_buttons assumes active
    if (isActive) {
        check_clip_buttons();
    } else {
        ui->button_set_clip_start->setEnabled(false);
        ui->button_set_clip_end->setEnabled(false);
    }

    //set menu actions enabled/disabled
    ui->actionExport->setEnabled(isActive);

    is_active = isActive;
}

/**
 * Set the appropriate depthmap settings from the application arguments.
 */
void QtOpenCVDepthmap::args_to_mapper() {
    mapper.numberOfDisparities = arguments.get_value<int>(Arguments::NUM_DISPARITIES);
    mapper.disp12MaxDiff = arguments.get_value<int>(Arguments::DISP12_MAX_DIFF);
    mapper.fullDP = arguments.get_value<bool>(Arguments::FULL_DP);
    mapper.minDisparity = arguments.get_value<int>(Arguments::MIN_DISPARITY);
    mapper.P1 = arguments.get_value<int>(Arguments::P1);
    mapper.P2 = arguments.get_value<int>(Arguments::P2);
    mapper.preFilterCap = arguments.get_value<int>(Arguments::PRE_FILTER_CAP);
    mapper.SADWindowSize = arguments.get_value<int>(Arguments::SAD_WINDOW_SIZE);
    mapper.speckleRange = arguments.get_value<int>(Arguments::SPECKLE_RANGE);
    mapper.speckleWindowSize = arguments.get_value<int>(Arguments::SPECKLE_WINDOW_SIZE);
    mapper.uniquenessRatio = arguments.get_value<int>(Arguments::UNIQUENESS);
}

/**
 * Open a video file for input.
 * @param filename The video file to process.
 */
void QtOpenCVDepthmap::open_filename(const std::string& filename) {
    feed_src.open(filename);
    if (feed_src.isOpened()) {
        arguments.set_value(Arguments::INPUT_FILENAME, filename);

        current_pos_msec = feed_src.get(CV_CAP_PROP_POS_MSEC);
        current_pos_frame = feed_src.get(CV_CAP_PROP_POS_FRAMES);
        current_pos_radio = feed_src.get(CV_CAP_PROP_POS_AVI_RATIO);
        input_frame_count = feed_src.get(CV_CAP_PROP_FRAME_COUNT);
        input_width = feed_src.get(CV_CAP_PROP_FRAME_WIDTH);
        input_height = feed_src.get(CV_CAP_PROP_FRAME_HEIGHT);
        input_fps = feed_src.get(CV_CAP_PROP_FPS);
        split_width = input_width * 0.5;

        output_width = split_width;
        output_height = input_height;
        output_fps = input_fps;

        int start_frame = arguments.get_value<int>(Arguments::START_FRAME);
        int end_frame = arguments.get_value<int>(Arguments::END_FRAME);
        if (!first_load) {
            start_frame = 1;
            end_frame = input_frame_count;
        } else {
            first_load = false;
        }
        correct_range(start_frame, 1, (int)input_frame_count-1);
        if (end_frame <= 0) {
            end_frame = input_frame_count;
        }
        correct_range(end_frame, start_frame, (int)input_frame_count);

        //set scrubber range
        ui->horizontalSlider->setRange(1, input_frame_count);
        //set spinbox ranges
        ui->spinBox_clip_start->setMaximum(input_frame_count);
        ui->spinBox_clip_end->setMaximum(input_frame_count);
        ui->spinBox_current_frame->setMaximum(input_frame_count);
        //set position defaults
        ui->horizontalSlider->setSliderPosition(1);
        ui->spinBox_current_frame->setValue(1);
        ui->spinBox_clip_end->setValue(end_frame);
        ui->spinBox_clip_start->setValue(start_frame);
        ui->label_total_frames->setText(QString::number(input_frame_count));

        //only set application as active after all settings have been loaded
        set_active(true);

        fetch_frame(start_frame);
    } else {
        QMessageBox msgBox;
        msgBox.setText(QString::fromStdString(filename) + " could not be opened for reading.");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
    }
}

//Input is 1-indexed
/**
 * Fetch and display the specified source frame.
 * @param index A 1-indexed value of the frame.
 */
void QtOpenCVDepthmap::fetch_frame(int index) {
    if (is_active) {
        //fetch and display source frame (0-indexed)
        feed_src.set(CV_CAP_PROP_POS_FRAMES, index-1);
        feed_src >> frame_src;
        ui->sbs_view->showImage(frame_src);

        update_depthmap();
    }
}

/**
 * Take the current input frame, process it, and display the depthmap.
 */
void QtOpenCVDepthmap::update_depthmap() {
    //compute depth map with current settings and display it
    left_eye = frame_src.colRange(0, split_width);
    right_eye = frame_src.colRange(split_width, input_width);
    mapper(left_eye, right_eye, frame_dst_16_gray);
    //the disparity mapper outputs CV_16UC1 when we need it in CV_8UC1
    frame_dst_16_gray.convertTo(frame_dst_8_gray, CV_8UC1);
    cvtColor(frame_dst_8_gray, frame_dst_8_colour, CV_GRAY2RGB);

    //display depthmap frame
    ui->depth_view->showImage(frame_dst_8_colour);
}

/**
 * Action to start an open-file dialog.
 */
void QtOpenCVDepthmap::on_actionOpen_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Open Video"), "", tr("Video Files (*.avi *.mpg *.mp4);;All Files (*.*)"));
    if (!filename.isNull()) {
        open_filename(filename.toStdString());
    }
}

/**
 * Change the specified parameter.
 * @param arg1 the new value.
 */
void QtOpenCVDepthmap::on_input_minDisparity_valueChanged(int arg1)
{
    update_mapper_value<int>(Arguments::MIN_DISPARITY, arg1);
}

/**
 * Change the specified parameter.
 * @param arg1 the new value.
 */
void QtOpenCVDepthmap::on_input_numDisparities_valueChanged(int arg1)
{
    update_mapper_value<int>(Arguments::NUM_DISPARITIES, arg1);
}

/**
 * Change the specified parameter.
 * @param arg1 the new value.
 */
void QtOpenCVDepthmap::on_input_SADWindowSize_valueChanged(int arg1)
{
    update_mapper_value<int>(Arguments::SAD_WINDOW_SIZE, arg1);
}

/**
 * Change the specified parameter.
 * @param arg1 the new value.
 */
void QtOpenCVDepthmap::on_input_P1_valueChanged(int arg1)
{
    //if return value is true, have to check both p1 and p2
    update_mapper_value<int>(Arguments::P1, arg1);
}

/**
 * Change the specified parameter.
 * @param arg1 the new value.
 */
void QtOpenCVDepthmap::on_input_P2_valueChanged(int arg1)
{
    //if return value is true, have to check both p1 and p2
    update_mapper_value<int>(Arguments::P2, arg1);
}

/**
 * Change the specified parameter.
 * @param arg1 the new value.
 */
void QtOpenCVDepthmap::on_input_disp12MAxDiff_valueChanged(int arg1)
{
    update_mapper_value<int>(Arguments::DISP12_MAX_DIFF, arg1);
}

/**
 * Change the specified parameter.
 * @param arg1 the new value.
 */
void QtOpenCVDepthmap::on_input_preFilterCap_valueChanged(int arg1)
{
    update_mapper_value<int>(Arguments::PRE_FILTER_CAP, arg1);
}

/**
 * Change the specified parameter.
 * @param arg1 the new value.
 */
void QtOpenCVDepthmap::on_input_uniqunessRatio_valueChanged(int arg1)
{
    update_mapper_value<int>(Arguments::UNIQUENESS, arg1);
}

/**
 * Change the specified parameter.
 * @param arg1 the new value.
 */
void QtOpenCVDepthmap::on_input_speckleWindowSize_valueChanged(int arg1)
{
    update_mapper_value<int>(Arguments::SPECKLE_WINDOW_SIZE, arg1);
}

/**
 * Change the specified parameter.
 * @param arg1 the new value.
 */
void QtOpenCVDepthmap::on_input_speckleRange_valueChanged(int arg1)
{
    update_mapper_value<int>(Arguments::SPECKLE_RANGE, arg1);
}

/**
 * Change the specified parameter.
 * @param arg1 the new value.
 */
void QtOpenCVDepthmap::on_input_fullDP_stateChanged(int arg1)
{
    /*
     Constant           Value   Description
     Qt::Unchecked          0   The item is unchecked.
     Qt::PartiallyChecked   1   The item is partially checked. Items in hierarchical models may be partially checked if some, but not all, of their children are checked.
     Qt::Checked            2   The item is checked.
    */
    update_mapper_value<bool>(Arguments::FULL_DP, (arg1 != 0) );
}

/**
 * The user has changed the current preview frame.
 * @param frame_index The new frame to display.
 */
void QtOpenCVDepthmap::on_horizontalSlider_valueChanged(int frame_index)
{
    if (is_active) {
        //std::cout<<"Frame: " << frame_index << "/" << input_frame_count << std::endl;
        fetch_frame(frame_index);
    }
}

//function to force a value into a range (inclusive)
/**
 * Caps a value into a range.
 * @param value The value to cap.
 * @param min The minimum Value can be.
 * @param max The maximum Value can be.
 */
template<typename T>
void QtOpenCVDepthmap::correct_range(T& value, T min, T max) {
    value = std::min(std::max(value, min), max);
}

/**
 * Handles the logic for modifying the end frame sub-range.
 * @param value Which frame should mark the end of the processing range.
 */
void QtOpenCVDepthmap::check_end_frame(int value) {
    int start_frame = ui->spinBox_clip_start->value();

    int end_frame = value;
    correct_range(end_frame, start_frame, (int)input_frame_count);

    update_mapper_value<int>(Arguments::END_FRAME, value);

    ui->horizontalSlider->sub_range_end_changed(end_frame);
    ui->spinBox_clip_end->setValue(end_frame);
    check_clip_buttons();
}

/**
 * Handles the logic for modifying the start frame sub-range.
 * @param value Which frame should mark the start of the processing range.
 */
void QtOpenCVDepthmap::check_start_frame(int value) {
    int end_frame = ui->spinBox_clip_end->value();

    int start_frame = value;
    correct_range(start_frame, 1, end_frame);

    update_mapper_value<int>(Arguments::START_FRAME, value);

    ui->horizontalSlider->sub_range_start_changed(start_frame);
    ui->spinBox_clip_start->setValue(start_frame);
    check_clip_buttons();
}

/**
 * Handles the logic for setting the current frame.
 * @param value Which frame to try and set the current frame to.
 */
void QtOpenCVDepthmap::check_current_frame(int value) {
    int original_spin = ui->spinBox_current_frame->value();
    int original_slider = ui->horizontalSlider->value();

    int current_frame = value;
    correct_range(current_frame, 1, (int)input_frame_count);

    if (current_frame != original_spin) {
        ui->spinBox_current_frame->setValue(current_frame);
    }
    if (current_frame != original_slider) {
        ui->horizontalSlider->setValue(current_frame);
    }
    check_clip_buttons();
}

/**
 * Checks to see if the current frame is a valid value to set as the beginning or end of a clip range.
 * Set the GUI controls accordingly. Assumes the application is active.
 */
void QtOpenCVDepthmap::check_clip_buttons() {
    int start_frame = arguments.get_value<int>(Arguments::START_FRAME);
    int end_frame = arguments.get_value<int>(Arguments::END_FRAME);
    int current_frame = ui->horizontalSlider->value();
    ui->button_set_clip_start->setEnabled(current_frame <= end_frame);
    ui->button_set_clip_end->setEnabled(current_frame >= start_frame);
}

/**
 * Quit the application.
 */
void QtOpenCVDepthmap::on_actionQuit_triggered()
{
    this->close();
}

/**
 * The user has chosen to export the current selection with the current settings.
 * Request an output video filename, and process it.
 */
void QtOpenCVDepthmap::on_actionExport_triggered()
{
    std::string output_filename = arguments.get_value<std::string>(Arguments::OUTPUT_FILENAME);
    QString filename = QFileDialog::getSaveFileName(this, tr("Save Video"), output_filename.c_str(), tr("Video Files (*.avi *.mpg *.mp4);;All Files (*.*)"));

    if (!filename.isNull()) {
        if (filename.toStdString() != output_filename) {
            arguments.set_value<std::string>(Arguments::OUTPUT_FILENAME, filename.toStdString());
        }

        QString exportLabel = "Exporting ";
        exportLabel.append(filename);

        Processor processor(arguments, this->feed_src);
        std::shared_ptr<cv::VideoWriter> output = processor.create_writer();

        size_t start_frame = arguments.get_value<int>(Arguments::START_FRAME);
        size_t end_frame   = arguments.get_value<int>(Arguments::END_FRAME);
        size_t range = end_frame + 1 - start_frame;

        //set up progress dialog
        QProgressDialog progress(exportLabel, "Cancel", 0, range, this);
        progress.setWindowModality(Qt::WindowModal);

        processor.set_next_frame(start_frame);

        for (size_t index = start_frame; index <= end_frame && !progress.wasCanceled(); ++index) {
            progress.setValue(index - start_frame);
            processor.process_next_frame(*output);
        }
        progress.setValue(range);
    }
}

/**
 * The user has specified that the current frame should be the start of the output clip.
 */
void QtOpenCVDepthmap::on_button_set_clip_start_clicked()
{
    check_start_frame(ui->horizontalSlider->value());
}

/**
 * The user has sepecified that the current frame should be the end of the output clip.
 */
void QtOpenCVDepthmap::on_button_set_clip_end_clicked()
{
    check_end_frame(ui->horizontalSlider->value());
}

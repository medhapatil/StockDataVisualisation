#include "ofApp.h"
#include <fstream>
#include <iostream>
//--------------------------------------------------------------
void ofApp::setup() {
    //Stocks scrollview
    stocknames_title = new ofxDatGuiLabel("SELECT A STOCK");
    stocknames_title->setPosition(0, 2);
    stocknames_title->setLabelAlignment(ofxDatGuiAlignment::CENTER);
    stocknames_scrll = new ofxDatGuiScrollView("Stocks", 24);           //Scrollview has 24 stocks viewable at a time
    stocknames_scrll->setPosition(0, stocknames_title->getY() + stocknames_title->getHeight() + 1);
    stocknames_scrll->onScrollViewEvent(this, &ofApp::onScrollViewEvent);
    
    //Intraday button
    intraday_button = new ofxDatGuiButton("LOAD INTRADAY DATA");
    intraday_button->setPosition(0, 682);
    intraday_button->setLabelAlignment(ofxDatGuiAlignment::CENTER);
    intraday_button->onButtonEvent(this, &ofApp::onIntradayButtonEvent);
    
    //Historical data button
    hist_button = new ofxDatGuiButton("LOAD HISTORICAL DATA");
    hist_button->setPosition(0, intraday_button->getY() + intraday_button->getHeight() + 1);
    hist_button->setLabelAlignment(ofxDatGuiAlignment::CENTER);
    hist_button->onButtonEvent(this, &ofApp::onHistoricalButtonEvent);
    
    //View in lines or points toggles
    view_lines_toggle = new ofxDatGuiToggle("VIEW IN LINES", true);
    view_lines_toggle->setPosition(0, hist_button->getY() + hist_button->getHeight() + 6);
    view_lines_toggle->setLabelAlignment(ofxDatGuiAlignment::CENTER);
    view_lines_toggle->onToggleEvent(this, &ofApp::onViewLinesEvent);
    view_points_toggle = new ofxDatGuiToggle("VIEW IN POINTS");
    view_points_toggle->setPosition(0, view_lines_toggle->getY() + view_lines_toggle->getHeight() + 1);
    view_points_toggle->setLabelAlignment(ofxDatGuiAlignment::CENTER);
    view_points_toggle->onToggleEvent(this, &ofApp::onViewPointsEvent);
    
    //Intraday data toggle folder
    intra_data_gui = new ofxDatGui(kWIDTH + 2, 2);
    intra_data_gui->addHeader("DISPLAY INTRADAY DATA",false);
    show_id_open = intra_data_gui->addToggle("View open prices", true);
    show_id_close = intra_data_gui->addToggle("View close prices", true);
    show_id_high = intra_data_gui->addToggle("View high prices", true);
    show_id_low = intra_data_gui->addToggle("View low prices", true);
    intra_data_gui->addFooter();
    intra_data_gui->getFooter()->setLabelWhenExpanded("CLICK TO CLOSE");
    intra_data_gui->getFooter()->setLabelWhenCollapsed("DISPLAY INTRADAY DATA");
    intra_data_gui->collapse();
    
    //Toggles for intraday data
    show_id_open->onToggleEvent(this, &ofApp::onToggleOpenEvent);
    show_id_close->onToggleEvent(this, &ofApp::onToggleCloseEvent);
    show_id_high->onToggleEvent(this, &ofApp::onToggleHighEvent);
    show_id_low->onToggleEvent(this, &ofApp::onToggleLowEvent);
    
    //Historical data toggle folder
    hist_data_gui = new ofxDatGui(intra_data_gui->getHeader()->getX() + kWIDTH + 2 , 2);
    hist_data_gui->addHeader("DISPLAY HISTORICAL DATA",false);
    show_hist_open = hist_data_gui->addToggle("View open prices", true);
    show_hist_close = hist_data_gui->addToggle("View close prices", true);
    show_hist_high = hist_data_gui->addToggle("View high prices", true);
    show_hist_low = hist_data_gui->addToggle("View low prices", true);
    hist_data_gui->addFooter();
    hist_data_gui->getFooter()->setLabelWhenExpanded("CLICK TO CLOSE");
    hist_data_gui->getFooter()->setLabelWhenCollapsed("DISPLAY HISTORICAL DATA");
    hist_data_gui->collapse();
    
    //Toggles for historical data
    show_hist_open->onToggleEvent(this, &ofApp::onToggleHistOpenEvent);
    show_hist_close->onToggleEvent(this, &ofApp::onToggleHistCloseEvent);
    show_hist_high->onToggleEvent(this, &ofApp::onToggleHistHighEvent);
    show_hist_low->onToggleEvent(this, &ofApp::onToggleHistLowEvent);
    
    //Daily data toggle folder
    daily_data_gui = new ofxDatGui(hist_data_gui->getHeader()->getX() + kWIDTH + 2, 2);
    daily_data_gui->addHeader("DISPLAY TODAY'S DATA");
    show_current_price = daily_data_gui->addToggle("View current price");
    curr_price_label = daily_data_gui->addLabel("");
    show_open = daily_data_gui->addToggle("View open price today");
    open_label = daily_data_gui->addLabel("");
    show_close = daily_data_gui->addToggle("View last close price");
    close_label = daily_data_gui->addLabel("");
    show_high = daily_data_gui->addToggle("View high price today");
    high_label = daily_data_gui->addLabel("");
    show_low = daily_data_gui->addToggle("View low price today");
    low_label = daily_data_gui->addLabel("");
    daily_data_gui->addBreak();
    show_year_high = daily_data_gui->addButton("View year high price");
    year_high_label = daily_data_gui->addLabel("");
    show_year_low = daily_data_gui->addButton("View year low price");
    year_low_label = daily_data_gui->addLabel("");
    daily_data_gui->addFooter();
    daily_data_gui->getFooter()->setLabelWhenExpanded("CLICK TO CLOSE");
    daily_data_gui->getFooter()->setLabelWhenCollapsed("DISPLAY TODAY'S DATA");
    daily_data_gui->collapse();
    
    //Toggles for daily data
    show_current_price->onToggleEvent(this, &ofApp::onShowCurrPriceEvent);
    show_open->onToggleEvent(this, &ofApp::onShowOpenEvent);
    show_close->onToggleEvent(this, &ofApp::onShowCloseEvent);
    show_high->onToggleEvent(this, &ofApp::onShowHighEvent);
    show_low->onToggleEvent(this, &ofApp::onShowLowEvent);
    show_year_high->onButtonEvent(this, &ofApp::onShowYearHighEvent);
    show_year_low->onButtonEvent(this, &ofApp::onShowYearLowEvent);
    
    //Change background color picker
    bg_color_picker = new ofxDatGuiColorPicker("CHANGE BG COLOR", ofColor::black);
    bg_color_picker->setPosition(daily_data_gui->getHeader()->getX() + kWIDTH + 2, 2);
    bg_color_picker->onColorPickerEvent(this, &ofApp::onBackgroundColorPickerEvent);
    
    //If daily data throws an error, message will be shown
    daily_data_error = new ofxDatGuiLabel("Daily data not available currently!");
    daily_data_error->setPosition(hist_data_gui->getHeader()->getX() + kWIDTH + 2, 15);
    daily_data_error->setStripeVisible(false);
    daily_data_error->setLabelAlignment(ofxDatGuiAlignment::CENTER);
    daily_data_na = false;
    data_na = false;    //If intraday and historical data is unavailable
    //Setting parameters for gui
    multicolor_bg = false;
    fullscreen = true;
    bg_color = ofColor(192,192,192);
    
    ParseNamesFile();       //Reading json file of names and parse it
    int i = 0;
    while (i != name_data["Name"].size()) {
        if (name_data["Name"][i] == "N/A") {
            i++;
            continue;
        }
        stocknames_scrll->add(name_data["Name"][i]);        //Adding names to scrollview
        i++;
    }

    //Plotting data
    all_plot.setPos(280, 32);
    all_plot.setDim(890, 650);
    all_plot.setTitleText("Stock data");
    all_plot.getTitle().setFontSize(12);
    all_plot.getTitle().setFontColor(0);
    all_plot.getYAxis().setAxisLabelText("Price in USD");
    all_plot.getXAxis().setNTicks(10);
    all_plot.activatePanning();
    all_plot.activateZooming(1.1, OF_MOUSE_BUTTON_LEFT, OF_MOUSE_BUTTON_LEFT);
    all_plot.activatePointLabels();
}
//--------------------------------------------------------------
void ofApp::update() {
    stocknames_scrll->update();
    stocknames_title->update();
    bg_color_picker->update();
    bg_color_picker->setWidth(190, 100);
    bg_color_picker->setStripeVisible(false);
    intraday_button->update();
    hist_button->update();
    view_lines_toggle->update();
    view_points_toggle->update();
    if (daily_data_na) {
        daily_data_error->update();
    }
    if (multicolor_bg) {
        cIndex = cIndex < bg_colors.size()-1 ? cIndex+1 : 0;
        ofBackground(bg_colors[cIndex]);
    }
}
//--------------------------------------------------------------
void ofApp::draw() {
    stocknames_scrll->draw();
    stocknames_title->draw();
    intraday_button->draw();
    hist_button->draw();
    view_lines_toggle->draw();
    view_points_toggle->draw();

    all_plot.beginDraw();       //Drawing data graph
    all_plot.drawBox();
    all_plot.drawTitle();
    all_plot.drawXAxis();
    all_plot.drawYAxis();
    all_plot.drawGridLines(GRAFICA_VERTICAL_DIRECTION);
    if (stock_is_selected) {
        if (id_layers_added && hist_layers_added) {
            //cout << "ERROR: Both at the same time" << endl;
        } else if (id_layers_added) {
            if (view_lines_toggle->getChecked()) {
                drawIntraDataLines();
            } else {
                drawIntraDataPoints();
            }
        } else if (hist_layers_added) {
            if (view_lines_toggle->getChecked()) {
                drawHistDataLines();
            } else {
                drawHistDataPoints();
            }
        }
        drawDailyDataLines();
    }
    all_plot.drawLabels();
    all_plot.endDraw();
    
    if (daily_data_na) {
        daily_data_error->draw();
    }
    if (data_na) {
        ofSetColor(0, 0, 0);
        ofDrawBitmapString("Some data points not available currently", 947, 656);
    }
    bg_color_picker->draw();
}
//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    if (key == 32) {    //Press SPACE to change themes
        tIndex = tIndex < themes.size()-1 ? tIndex+1 : 0;
        daily_data_gui->setTheme(themes[tIndex]);
        intra_data_gui->setTheme(themes[tIndex]);
        hist_data_gui->setTheme(themes[tIndex]);
        bg_color_picker->setTheme(themes[tIndex]);
        stocknames_title->setTheme(themes[tIndex]);
        stocknames_scrll->setTheme(themes[tIndex]);
        intraday_button->setTheme(themes[tIndex]);
        hist_button->setTheme(themes[tIndex]);
        view_lines_toggle->setTheme(themes[tIndex]);
        view_points_toggle->setTheme(themes[tIndex]);
    }
    if (key == OF_KEY_SHIFT) {  //Press SHIFT to change to multicolor background
        multicolor_bg = !multicolor_bg;
        if (!multicolor_bg) {
            ofBackground(bg_color);
        }
    }
    if (key == 'f') {   //Press F to toggle fullscreen
        fullscreen = !fullscreen;
        ofSetFullscreen(fullscreen);
    }
}
//--------------------------------------------------------------
void ofApp::onScrollViewEvent(ofxDatGuiScrollViewEvent e) {
    stock_selected = name_data["Symbol"][e.target->getIndex()];
    name_stock_selected = name_data["Name"][e.target->getIndex()];
    stock_is_selected = true;
    if (id_layers_added) {
        RemoveIntraLayers();
        id_layers_added = false;
    }
    if (hist_layers_added) {
        RemoveHistLayers();
        hist_layers_added = false;
    }
    GetDailyData();
    all_plot.setTitleText("Stock data");
    all_plot.getXAxis().setAxisLabelText("");
}
//--------------------------------------------------------------
void ofApp::onViewLinesEvent(ofxDatGuiToggleEvent e) {
    view_lines_toggle->setChecked(true);
    view_points_toggle->setChecked(false);
}

void ofApp::onViewPointsEvent(ofxDatGuiToggleEvent e) {
    view_lines_toggle->setChecked(false);
    view_points_toggle->setChecked(true);
}
//--------------------------------------------------------------
void ofApp::onIntradayButtonEvent(ofxDatGuiButtonEvent e) {
    all_plot.activateReset();
    if (hist_layers_added) {
        RemoveHistLayers();
        hist_layers_added = false;
    }
    GetIntradayData();
    if (!id_layers_added) {
        all_plot.addLayer("Open points", id_open_points);
        all_plot.addLayer("Close points", id_close_points);
        all_plot.addLayer("High points", id_high_points);
        all_plot.addLayer("Low points", id_low_points);
    }
    id_layers_added = true;
    all_plot.setTitleText("Intraday data for " + name_stock_selected + " on " + intraday_date);
    all_plot.getXAxis().setAxisLabelText("Hours of day");
}

void ofApp::onHistoricalButtonEvent(ofxDatGuiButtonEvent e) {
    all_plot.activateReset();
    if (id_layers_added) {
        RemoveIntraLayers();
        id_layers_added = false;
    }
    GetHistoricalData();
    if (!hist_layers_added) {
        all_plot.addLayer("Open Hist points", hist_open_points);
        all_plot.addLayer("Close Hist points", hist_close_points);
        all_plot.addLayer("High Hist points", hist_high_points);
        all_plot.addLayer("Low Hist points", hist_low_points);
    }
    hist_layers_added = true;
    all_plot.setTitleText("Historical data for " + name_stock_selected);
    all_plot.getXAxis().setAxisLabelText("Year");
}
//--------------------------------------------------------------
void ofApp::ParseNamesFile() {
    std::ifstream file;
    file.open(kFILEPATH_JSON);
    name_data = json::parse(file);
    file.close();
}

void ofApp::GetDailyData() {
    if (stock_is_selected) {
        single_day_url = "https://www.worldtradingdata.com/api/v1/stock?symbol=" + stock_selected + "&api_token=" + kAPI_KEY;
        ofHttpResponse resp = ofLoadURL(single_day_url);
        single_day_data = json::parse(resp.data);
        cp_label = single_day_data["data"][0]["price"];
        op_label = single_day_data["data"][0]["price_open"];
        dh_label = single_day_data["data"][0]["day_high"];
        dl_label = single_day_data["data"][0]["day_low"];
        cl_label = single_day_data["data"][0]["close_yesterday"];
        year_high = single_day_data["data"][0]["52_week_high"];
        year_low = single_day_data["data"][0]["52_week_low"];
        if (cp_label == "N/A" || op_label == "N/A" || dh_label == "N/A" || dl_label == "N/A"
            || cl_label == "N/A") {
            daily_data_na = true;
        } else {
            daily_data_na = false;
            current_price = stof(cp_label);
            price_open = stof(op_label);
            day_high = stof(dh_label);
            day_low = stof(dl_label);
            close_yesterday = stof(cl_label);
        }
    }
}

void ofApp::GetIntradayData() {
    if (stock_is_selected) {
        intraday_url = "https://www.worldtradingdata.com/api/v1/intraday?symbol=" + stock_selected + "&range=1&interval=1&api_token=" + kAPI_KEY;
        ofHttpResponse id_resp = ofLoadURL(intraday_url);
        intraday_json = json::parse(id_resp.data);
        json js = intraday_json["intraday"];
        string d = js.dump();
        intra_data_set = jsoncons::decode_json<dated_data_collection>(d);
        ClearPoints();
        bool date_noted = false;
        for (const auto& time_stamp_data : intra_data_set) {
            string time_stamp = time_stamp_data.first;
            if (!date_noted) {
                intraday_date = time_stamp.substr(5,2) + "/" + time_stamp.substr(8,2) + "/" + time_stamp.substr(0,4);
                date_noted = true;
            }
            string info = time_stamp_data.second;
            json j = json::parse(info);
            string open = j["open"];
            string close = j["close"];
            string high = j["high"];
            string low = j["low"];
            if (open == "N/A" || close == "N/A" || high == "N/A" || low == "N/A") {
                data_na = true;
            } else {
                int hour = stoi(time_stamp.substr(11,2));
                int min = stoi(time_stamp.substr(14,2));
                float time = hour + (min / 60.0);
                id_open_points.emplace_back(time, stof(open), "(" + time_stamp.substr(11,5) + ", " + open + ")");
                id_close_points.emplace_back(time, stof(close), "(" + time_stamp.substr(11,5) + ", " + close + ")");
                id_high_points.emplace_back(time, stof(high), "(" + time_stamp.substr(11,5) + ", " + high + ")");
                id_low_points.emplace_back(time, stof(low), "(" + time_stamp.substr(11,5) + ", " + low + ")");
                num_id_points++;
            }
        }
    }
}

void ofApp::GetHistoricalData() {
    if (stock_is_selected) {
        all_url = "https://www.worldtradingdata.com/api/v1/history?symbol=" + stock_selected + "&api_token=" + kAPI_KEY;
        ofHttpResponse h_resp = ofLoadURL(all_url);
        all_json = json::parse(h_resp.data);
        json js = all_json["history"];
        string d = js.dump();
        all_data_set = jsoncons::decode_json<dated_data_collection>(d);
        ClearPoints();
        for (const auto& time_stamp_data : all_data_set) {
            string time_stamp = time_stamp_data.first;
            string info = time_stamp_data.second;
            json j = json::parse(info);
            string open = j["open"];
            string close = j["close"];
            string high = j["high"];
            string low = j["low"];
            string volume = j["volume"];
            if (open == "N/A" || close == "N/A" || high == "N/A" || low == "N/A" || volume == "N/A") {
                data_na = true;
            } else {
                int year = stoi(time_stamp.substr(0,4));
                int month = stoi(time_stamp.substr(5,2));
                int day = stoi(time_stamp.substr(8,2));
                float date = GetDateAsFloat(year, month, day);
                string date_as_string = time_stamp.substr(5,2) + "/" + time_stamp.substr(8,2) + "/" + time_stamp.substr(0,4);
                hist_open_points.emplace_back(date, stof(open), "(" + date_as_string + ", " + open + ")");
                hist_close_points.emplace_back(date, stof(close), "(" + date_as_string + ", " + close + ")");
                hist_high_points.emplace_back(date, stof(high), "(" + date_as_string + ", " + high + ")");
                hist_low_points.emplace_back(date, stof(low), "(" + date_as_string + ", " + low + ")");
                num_hist_points++;
            }
        }
    }
}
//--------------------------------------------------------------
float ofApp::GetDateAsFloat(int year, int month, int day) {
    bool leap_year = false;
    vector<int> days_per_month = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    vector<int> days_per_month_leap_year = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (year % 400 == 0) {
        leap_year = true;
    } else if (year % 100 == 0) {
        leap_year = false;
    } else if (year % 4 == 0) {
        leap_year = true;
    }
    if (leap_year) {
        return year + (month + (day - 1.0) / days_per_month_leap_year[month]) / 12.0;
    } else {
        return year + (month + (day - 1.0) / days_per_month[month]) / 12.0;
    }
}
//--------------------------------------------------------------
void ofApp::ClearPoints() {
    id_open_points.clear();
    id_close_points.clear();
    id_high_points.clear();
    id_low_points.clear();
    num_id_points = 0;
    hist_open_points.clear();
    hist_close_points.clear();
    hist_high_points.clear();
    hist_low_points.clear();
    num_hist_points = 0;
}

void ofApp::RemoveIntraLayers() {
    all_plot.removeLayer("Open points");
    all_plot.removeLayer("Close points");
    all_plot.removeLayer("High points");
    all_plot.removeLayer("Low points");
}

void ofApp::RemoveHistLayers() {
    all_plot.removeLayer("Open Hist points");
    all_plot.removeLayer("Close Hist points");
    all_plot.removeLayer("High Hist points");
    all_plot.removeLayer("Low Hist points");
}
//--------------------------------------------------------------
void ofApp::onToggleOpenEvent(ofxDatGuiToggleEvent e) {}
void ofApp::onToggleCloseEvent(ofxDatGuiToggleEvent e) {}
void ofApp::onToggleHighEvent(ofxDatGuiToggleEvent e) {}
void ofApp::onToggleLowEvent(ofxDatGuiToggleEvent e) {}
void ofApp::onToggleHistOpenEvent(ofxDatGuiToggleEvent e) {}
void ofApp::onToggleHistCloseEvent(ofxDatGuiToggleEvent e) {}
void ofApp::onToggleHistHighEvent(ofxDatGuiToggleEvent e) {}
void ofApp::onToggleHistLowEvent(ofxDatGuiToggleEvent e) {}
//--------------------------------------------------------------
void ofApp::onShowCurrPriceEvent(ofxDatGuiToggleEvent e) {
    curr_price_label->setLabel(cp_label);
}

void ofApp::onShowOpenEvent(ofxDatGuiToggleEvent e) {
    open_label->setLabel(op_label);
}

void ofApp::onShowCloseEvent(ofxDatGuiToggleEvent e) {
    close_label->setLabel(cl_label);
}

void ofApp::onShowHighEvent(ofxDatGuiToggleEvent e) {
    high_label->setLabel(dh_label);
}

void ofApp::onShowLowEvent(ofxDatGuiToggleEvent e) {
    low_label->setLabel(dl_label);
}

void ofApp::onShowYearHighEvent(ofxDatGuiButtonEvent e) {
    year_high_label->setLabel(year_high);
}

void ofApp::onShowYearLowEvent(ofxDatGuiButtonEvent e) {
    year_low_label->setLabel(year_low);
}
//--------------------------------------------------------------
void ofApp::drawIntraDataLines() {
    for (int i = 0; i < num_id_points - 1; ++i) {
        if (show_id_open->getChecked()) {
            all_plot.drawLine(id_open_points[i], id_open_points[i + 1], ofColor(0, 130, 0), 2);
        }
        if (show_id_close->getChecked()) {
            all_plot.drawLine(id_close_points[i], id_close_points[i + 1], ofColor(0, 0, 130), 2);
        }
        if (show_id_low->getChecked()) {
            all_plot.drawLine(id_low_points[i], id_low_points[i + 1], ofColor(130, 0, 0), 2);
        }
        if (show_id_high->getChecked()) {
            all_plot.drawLine(id_high_points[i], id_high_points[i + 1], ofColor(130, 0, 130), 2);
        }
    }
}
//--------------------------------------------------------------
void ofApp::drawHistDataLines() {
    for (int i = 0; i < num_hist_points - 1; ++i) {
        if (show_hist_open->getChecked()) {
            all_plot.drawLine(hist_open_points[i], hist_open_points[i + 1], ofColor(0, 130, 0), 2);
        }
        if (show_hist_close->getChecked()) {
            all_plot.drawLine(hist_close_points[i], hist_close_points[i + 1], ofColor(0, 0, 130), 2);
        }
        if (show_hist_low->getChecked()) {
            all_plot.drawLine(hist_low_points[i], hist_low_points[i + 1], ofColor(130, 0, 0), 2);
        }
        if (show_hist_high->getChecked()) {
            all_plot.drawLine(hist_high_points[i], hist_high_points[i + 1], ofColor(130, 0, 130), 2);
        }
    }
}
//--------------------------------------------------------------
void ofApp::drawIntraDataPoints() {
    for (int i = 0; i < num_id_points; ++i) {
        if (show_id_open->getChecked()) {
            all_plot.drawPoint(id_open_points[i], ofColor(0, 130, 0), 2);
        }
        if (show_id_close->getChecked()) {
            all_plot.drawPoint(id_close_points[i], ofColor(0, 0, 130), 2);
        }
        if (show_id_low->getChecked()) {
            all_plot.drawPoint(id_low_points[i], ofColor(130, 0, 0), 2);
        }
        if (show_id_high->getChecked()) {
            all_plot.drawPoint(id_high_points[i], ofColor(130, 0, 130), 2);
        }
    }
}
//--------------------------------------------------------------
void ofApp::drawHistDataPoints() {
    for (int i = 0; i < num_hist_points; ++i) {
        if (show_hist_open->getChecked()) {
            all_plot.drawPoint(hist_open_points[i], ofColor(0, 130, 0), 2);
        }
        if (show_hist_close->getChecked()) {
            all_plot.drawPoint(hist_close_points[i], ofColor(0, 0, 130), 2);
        }
        if (show_hist_low->getChecked()) {
            all_plot.drawPoint(hist_low_points[i], ofColor(130, 0, 0), 2);
        }
        if (show_hist_high->getChecked()) {
            all_plot.drawPoint(hist_high_points[i], ofColor(130, 0, 130), 2);
        }
    }
}
//--------------------------------------------------------------
void ofApp::drawDailyDataLines() {
    if (show_current_price->getChecked()) {
        all_plot.drawHorizontalLine(current_price, 0, 1);
    }
    if (show_open->getChecked()) {
        all_plot.drawHorizontalLine(price_open, 0, 1);
    }
    if (show_close->getChecked()) {
        all_plot.drawHorizontalLine(close_yesterday, 0, 1);
    }
    if (show_high->getChecked()) {
        all_plot.drawHorizontalLine(day_high, 0, 1);
    }
    if (show_low->getChecked()) {
        all_plot.drawHorizontalLine(day_low, 0, 1);
    }
}
//--------------------------------------------------------------
void ofApp::onBackgroundColorPickerEvent(ofxDatGuiColorPickerEvent e) {
    bg_color = e.color;
    ofBackground(bg_color);
}

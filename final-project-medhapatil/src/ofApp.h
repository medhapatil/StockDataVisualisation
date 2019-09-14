#pragma once
#include "ofMain.h"
#include "ofxDatGui.h"              //Using DatGui addon in Openframeworks for GUI
#include "ofxGrafica.h"             //Using Grafica addon in Openframeworks for plotting graphs
#include <map>
#include <jsoncons/json.hpp>        //Using jsoncons library to manipulate json in C++
#include "json.hpp"                 //Using nlohmann json library to parse json in C++
using json = nlohmann::json;

class ofApp : public ofBaseApp {
private:
    const string kAPI_KEY = "q3VCn4olCiEvaXanAwZa2ZdlnnXxmDoQCEaWDyUmKvBwh0L6Ew5hkzJxXQj6";
    const string kFILEPATH_JSON = "/Applications/OpenFrameworks/apps/myApps/final-project-medhapatil/final-project-medhapatil/bin/data/stockdata.json";
    const int kWIDTH = 270;
public:
    void setup();
    void update();
    void draw();
    void keyPressed(int key);
    
    //Stock names scrollview
    ofxDatGuiScrollView* stocknames_scrll;
    ofxDatGuiLabel* stocknames_title;
    void onScrollViewEvent(ofxDatGuiScrollViewEvent e);
    
    //Buttons for today's data or historical data
    ofxDatGuiButton* intraday_button;
    ofxDatGuiButton* hist_button;
    void onIntradayButtonEvent(ofxDatGuiButtonEvent e);
    void onHistoricalButtonEvent(ofxDatGuiButtonEvent e);
    
    //GUI for daily data
    ofxDatGui* daily_data_gui;
    ofxDatGuiToggle* show_open;
    ofxDatGuiToggle* show_close;
    ofxDatGuiToggle* show_high;
    ofxDatGuiToggle* show_low;
    ofxDatGuiToggle* show_current_price;
    ofxDatGuiButton* show_year_high;
    ofxDatGuiButton* show_year_low;
    ofxDatGuiLabel* curr_price_label;
    ofxDatGuiLabel* open_label;
    ofxDatGuiLabel* high_label;
    ofxDatGuiLabel* low_label;
    ofxDatGuiLabel* close_label;
    ofxDatGuiLabel* year_high_label;
    ofxDatGuiLabel* year_low_label;
    void onShowCurrPriceEvent(ofxDatGuiToggleEvent e);
    void onShowOpenEvent(ofxDatGuiToggleEvent e);
    void onShowHighEvent(ofxDatGuiToggleEvent e);
    void onShowLowEvent(ofxDatGuiToggleEvent e);
    void onShowCloseEvent(ofxDatGuiToggleEvent e);
    void onShowYearHighEvent(ofxDatGuiButtonEvent e);
    void onShowYearLowEvent(ofxDatGuiButtonEvent e);
    
    //GUI for intraday data
    ofxDatGui* intra_data_gui;
    ofxDatGuiToggle* show_id_open;
    ofxDatGuiToggle* show_id_close;
    ofxDatGuiToggle* show_id_high;
    ofxDatGuiToggle* show_id_low;
    void onToggleOpenEvent(ofxDatGuiToggleEvent e);
    void onToggleHighEvent(ofxDatGuiToggleEvent e);
    void onToggleLowEvent(ofxDatGuiToggleEvent e);
    void onToggleCloseEvent(ofxDatGuiToggleEvent e);
    
    //GUI for historical data
    ofxDatGui* hist_data_gui;
    ofxDatGuiToggle* show_hist_open;
    ofxDatGuiToggle* show_hist_close;
    ofxDatGuiToggle* show_hist_high;
    ofxDatGuiToggle* show_hist_low;
    void onToggleHistOpenEvent(ofxDatGuiToggleEvent e);
    void onToggleHistHighEvent(ofxDatGuiToggleEvent e);
    void onToggleHistLowEvent(ofxDatGuiToggleEvent e);
    void onToggleHistCloseEvent(ofxDatGuiToggleEvent e);
    
    //Getting daily data
    void GetDailyData();
    float current_price;
    float price_open;
    float day_high;
    float day_low;
    float close_yesterday;
    bool daily_data_na;       //checks if data is unavailable
    string cp_label;         //displaying in labels
    string op_label;
    string dh_label;
    string dl_label;
    string cl_label;
    string year_high;
    string year_low;
    
    //Getting intraday and historical data
    typedef std::map<std::string,std::string> dated_data_collection;
    bool data_na;               //checks if data is unavailable
    void GetIntradayData();
    string intraday_date;
    dated_data_collection intra_data_set;
    void GetHistoricalData();
    dated_data_collection all_data_set;
    
    //Date manipulation for historical data
    static float GetDateAsFloat(int year, int month, int day);
    
    //Setup plot
    ofxGPlot all_plot;
    void drawIntraDataLines();      //Draw for plot
    void drawHistDataLines();
    void drawIntraDataPoints();
    void drawHistDataPoints();
    void drawDailyDataLines();
    
    //Plotting points
    bool id_layers_added;
    int num_id_points;
    vector<ofxGPoint> id_open_points;
    vector<ofxGPoint> id_close_points;
    vector<ofxGPoint> id_high_points;
    vector<ofxGPoint> id_low_points;
    bool hist_layers_added;
    int num_hist_points;
    vector<ofxGPoint> hist_open_points;
    vector<ofxGPoint> hist_close_points;
    vector<ofxGPoint> hist_high_points;
    vector<ofxGPoint> hist_low_points;
    
    //Json and Url building
    json name_data;             //Stock names data
    void ParseNamesFile();
    json single_day_data;       //Daily data
    string single_day_url;
    json intraday_json;         //Intraday data
    string intraday_url;
    json all_json;              //Historical data
    string all_url;
    string stock_selected;
    string name_stock_selected;
    bool stock_is_selected;
    
    //Clearing points and layers
    void ClearPoints();
    void RemoveIntraLayers();
    void RemoveHistLayers();
    
    //Option to view data in points or lines
    ofxDatGuiToggle* view_lines_toggle;
    ofxDatGuiToggle* view_points_toggle;
    void onViewLinesEvent(ofxDatGuiToggleEvent e);
    void onViewPointsEvent(ofxDatGuiToggleEvent e);
    
    //Error message label if no available data
    ofxDatGuiLabel* daily_data_error;
    ofxDatGuiLabel* data_error;
    
    //More GUI controls
    bool fullscreen;        //To toggle fullscreen
    ofxDatGuiColorPicker* bg_color_picker;      //Color picker for background color
    void onBackgroundColorPickerEvent(ofxDatGuiColorPickerEvent e);
    vector<ofxDatGuiTheme*> themes = { new ofxDatGuiTheme(true), new ofxDatGuiThemeSmoke(),
        new ofxDatGuiThemeWireframe(), new ofxDatGuiThemeMidnight(), new ofxDatGuiThemeAqua(),
        new ofxDatGuiThemeCharcoal(), new ofxDatGuiThemeAutumn(), new ofxDatGuiThemeCandy() };
    vector<ofColor> bg_colors = { ofColor(204,255,229), ofColor(204,255,255), ofColor(204,229,255),
        ofColor(204,204,255), ofColor(229,204,255), ofColor(255,204,255), ofColor(255,204,229),
        ofColor(255,204,204), ofColor(255,229,204), ofColor(255,255,204), ofColor(229,255,204),
        ofColor(204,255,204) };         //Colors for multicolor background
    uint tIndex = 0;    //variable for switching themes
    uint cIndex = 0;    //variable for switching bg colors
    bool multicolor_bg;
    ofColor bg_color;
};

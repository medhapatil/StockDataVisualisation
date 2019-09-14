# Stocks Visualisation App Final Proposal:

### About:
This application would be an interactive platform for visualising real-time data such as stock prices and currency exchange rates, based on parameters entered by the user. Currently, I intend to first work on only the stock time-series for companies and will extend to currency exchange rates if time permits. 

### Data:
Data will be taken by calls to worldtradingdata.com. Data can be retrieved using JSON files and the documentation is straightforward.

### Features:
* Users will be able to switch between different companies and see the values of their stocks graphed against time-series. These will be line graphs over a period of time chosen by user.
* Users will adjust parameters using buttons or drop down menus. 

### Libraries:
* Nlohmann JSON for modern C++ and JsonCons (by Daniela Parker) libraries for JSON parsing and manipulation
* ofxDatGui: for making platform interactive for users to enter parameters through toggles, scrollview, buttons etc.
* ofxGui: interface design
* ofxGrafica: interface design
* ofxHTTPResponse for making GET request for URL

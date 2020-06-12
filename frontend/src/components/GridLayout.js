import React, {Component} from 'react';
import {Responsive, WidthProvider} from 'react-grid-layout';
import Map from './Map';
import Parameters from './Parameters';
import Measures from './Measures';
import Results from './Results';
import Timeline from './Timeline';
import TimeMap from './TimeMap'

import './GridLayout.scss';

const ResponsiveGridLayout = WidthProvider(Responsive);

class ResponsiveGrid extends Component {
  render() {
    // {lg: layout1, md: layout2, ...}
    //const layouts = getLayoutsFromSomewhere();
    return (
      <>
        <div className="earth">
          <img src="assets/earth.png" alt="Welt"/>
        </div>
        <header>
          <span className="logo">
            <img
              src="assets/logo.png"
              alt="Deutsches Zentrum für Luft- und Raumfahrt (DLR)"
            />
            <span className="logo-text">
              Deutsches Zentrum
              <br/>
              für Luft- und Raumfahrt
            </span>
          </span>
        </header>
        <ResponsiveGridLayout
          className="layout position-relative"
          breakpoints={{lg: 1200, md: 996, sm: 768, xs: 480, xxs: 0}}
          cols={{lg: 20, md: 10, sm: 6, xs: 4, xxs: 2}}
          rowHeight={100}
          containerPadding={[10, 10]}
          margin={[5, 5]}
          autoSize={true}
          draggableHandle=".grid-draggable-handle"
        >
          <div className="grid-box"
               key="timeline"
               data-grid={{x: 0, y: 0, w: 6, h: 1}}>
            <i className="fas fa-arrows-alt grid-draggable-handle"/>
            <Timeline/>
          </div>
          <div className="grid-box"
               key="timeMap"
               data-grid={{x: 0, y: 1, w: 6, h: 7}}>
            <i className="fas fa-arrows-alt grid-draggable-handle"/>
            <TimeMap/>
          </div>
          <div
            className="grid-box"
            key="map"
            data-grid={{x: 6, y: 1, w: 6, h: 7}}
          >
            <i className="fas fa-arrows-alt grid-draggable-handle"></i>
            <Map/>
          </div>
          <div
            key="measures"
            data-grid={{x: 16, y: 4, w: 4, h: 3}}
            className="grid-box"
          >
            <i className="fas fa-arrows-alt grid-draggable-handle"></i>
            <Measures/>
          </div>
          <div
            key="parameters"
            data-grid={{x: 12, y: 4, w: 4, h: 3}}
            className="grid-box"
          >
            <i className="fas fa-arrows-alt grid-draggable-handle"></i>
            <Parameters/>
          </div>
          <div
            key="results"
            data-grid={{x: 12, y: 0, w: 8, h: 5}}
            className="grid-box"
          >
            <i className="fas fa-arrows-alt grid-draggable-handle"></i>
            <Results/>
          </div>
        </ResponsiveGridLayout>
      </>
    );
  }
}

export default ResponsiveGrid;

//
// Created by ulrich on 08.04.19.
//

#include "TouchInputHandler.h"

#include <cmath>

#include "InputContext.h"

TouchInputHandler::TouchInputHandler(InputContext* inputContext): AbstractInputHandler(inputContext) {}

auto TouchInputHandler::handleImpl(InputEvent const& event) -> bool {
    bool zoomGesturesEnabled = inputContext->getSettings()->isZoomGesturesEnabled();

    // Don't handle more then 2 inputs
    if (this->primarySequence && this->primarySequence != event.sequence && this->secondarySequence &&
        this->secondarySequence != event.sequence) {
        return false;
    }

    if (event.type == BUTTON_PRESS_EVENT) {
        // Start scrolling when a sequence starts and we currently have none other
        if (this->primarySequence == nullptr && this->secondarySequence == nullptr) {
            this->primarySequence = event.sequence;

            // Set sequence data
            sequenceStart(event);
        }
        // Start zooming as soon as we have two sequences.
        else if (this->primarySequence && this->primarySequence != event.sequence &&
                 this->secondarySequence == nullptr) {
            this->secondarySequence = event.sequence;

            // Even if zoom gestures are disabled,
            // this is still the start of a sequence.

            // Set sequence data
            sequenceStart(event);

            startZoomReady = true;
        }
    }

    if (event.type == MOTION_EVENT && this->primarySequence) {
        if (this->primarySequence && this->secondarySequence && zoomGesturesEnabled) {
            if (startZoomReady) {
                if (this->primarySequence == event.sequence) {
                    sequenceStart(event);
                    zoomStart();
                }
            } else {
                zoomMotion(event);
            }
        } else if (event.sequence == this->primarySequence) {
            scrollMotion(event);
        } else if (this->primarySequence && this->secondarySequence) {
            sequenceStart(event);
        }
    }

    if (event.type == BUTTON_RELEASE_EVENT) {
        // Only stop zooing if both sequences were active (we were scrolling)
        if (this->primarySequence != nullptr && this->secondarySequence != nullptr && zoomGesturesEnabled) {
            zoomEnd();
        }

        if (event.sequence == this->primarySequence) {
            // If secondarySequence is nullptr, this sets primarySequence
            // to nullptr. If it isn't, then it is now the primary sequence!
            this->primarySequence = this->secondarySequence;
            this->secondarySequence = nullptr;

            this->priLastAbs = this->secLastAbs;
            this->priLastRel = this->secLastRel;
        } else {
            this->secondarySequence = nullptr;
        }
    }

    return false;
}

void TouchInputHandler::sequenceStart(InputEvent const& event) {
    if (event.sequence == this->primarySequence) {
        this->priLastAbs = {event.absoluteX, event.absoluteY};
        this->priLastRel = {event.relativeX, event.relativeY};
    } else {
        this->secLastAbs = {event.absoluteX, event.absoluteY};
        this->secLastRel = {event.relativeX, event.relativeY};
    }
}

void TouchInputHandler::scrollMotion(InputEvent const& event) {
    // Will only be called if there is a single sequence (zooming handles two sequences)
    auto offset = [&]() {
        auto absolutePoint = utl::Point{event.absoluteX, event.absoluteY};
        if (event.sequence == this->primarySequence) {
            auto offset = absolutePoint - this->priLastAbs;
            this->priLastAbs = absolutePoint;
            return offset;
        } else {
            auto offset = absolutePoint - this->secLastAbs;
            this->secLastAbs = absolutePoint;
            return offset;
        }
    }();

    auto* layout = inputContext->getView()->getControl()->getWindow()->getLayout();

    layout->scrollRelative(-offset.x, -offset.y);
}

void TouchInputHandler::zoomStart() {
    this->startZoomDistance = this->priLastAbs.distance(this->secLastAbs);

    if (this->startZoomDistance == 0.0) {
        this->startZoomDistance = 0.01;
    }

    // Whether we can ignore the zoom portion of the gesture (e.g. distance between touch points
    // hasn't changed enough).
    this->canBlockZoom = true;

    lastZoomScrollCenter = (this->priLastAbs + this->secLastAbs) / 2.0;

    ZoomControl* zoomControl = this->inputContext->getView()->getControl()->getZoomControl();

    // Disable zoom fit as we are zooming currently
    // TODO(fabian): this should happen internally!!!
    if (zoomControl->isZoomFitMode()) {
        zoomControl->setZoomFitMode(false);
    }

    auto center = (this->priLastAbs + this->secLastAbs) / 2.0;

    auto* mainWindow = inputContext->getView()->getControl()->getWindow();
    // use screen pixel coordinates for the zoom center
    // as relative coordinates depend on the changing zoom level
    int rx, ry;
    gdk_window_get_root_coords(gtk_widget_get_window(mainWindow->getWinXournal()), 0, 0, &rx, &ry);
    center.x -= rx;
    center.y -= ry;

    // When not using touch drawing, we're using a different scrolling method.
    // This requires different centering.
    /*if (!mainWindow->getGtkTouchscreenScrollingEnabled()) {
        center = (this->priLastRel + this->secLastRel) / 2.0;  // TODO check!
    }*/

    zoomControl->startZoomSequence(center);

    startZoomReady = false;
}

void TouchInputHandler::zoomMotion(InputEvent const& event) {
    if (event.sequence == this->primarySequence) {
        this->priLastAbs = {event.absoluteX, event.absoluteY};
    } else {
        this->secLastAbs = {event.absoluteX, event.absoluteY};
    }

    double distance = this->priLastAbs.distance(this->secLastAbs);
    double zoom = distance / this->startZoomDistance;

    double zoomTriggerThreshold = inputContext->getSettings()->getTouchZoomStartThreshold();
    double zoomChangePercentage = std::abs(distance - startZoomDistance) / startZoomDistance * 100;

    // Has the touch points moved far enough to trigger a zoom?
    if (this->canBlockZoom && zoomChangePercentage < zoomTriggerThreshold) {
        zoom = 1.0;
    } else {
        // Touches have moved far enough from their initial location that we
        // no longer prevent touchscreen zooming.
        this->canBlockZoom = false;
    }

    ZoomControl* zoomControl = this->inputContext->getView()->getControl()->getZoomControl();
    auto center = (this->priLastAbs + this->secLastAbs) / 2;
    zoomControl->zoomSequenceChange(zoom, true, center - lastZoomScrollCenter);
    lastZoomScrollCenter = center;
}

void TouchInputHandler::zoomEnd() {
    ZoomControl* zoomControl = this->inputContext->getView()->getControl()->getZoomControl();
    zoomControl->endZoomSequence();
}

void TouchInputHandler::onUnblock() {
    this->primarySequence = nullptr;
    this->secondarySequence = nullptr;

    this->startZoomDistance = 0.0;
    this->lastZoomScrollCenter = {};

    priLastAbs = {-1.0, -1.0};
    secLastAbs = {-1.0, -1.0};
    priLastRel = {-1.0, -1.0};
    secLastRel = {-1.0, -1.0};
}

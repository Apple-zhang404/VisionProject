#include "Balser_Cam.h"

Balser_Cam::Balser_Cam() : m_bcamopen(false)
{
}

Balser_Cam::~Balser_Cam()
{
}

bool Balser_Cam::openCam()
{
    if (!m_bcamopen) {
        // TODO: open the Basler camera here
        m_bcamopen = true;
    }
    return m_bcamopen;
}

bool Balser_Cam::closeCam()
{
    if (!m_bcamopen) {
        return false;
    }
    m_bcamopen = false;
    return true;
}

bool Balser_Cam::getFrame()
{
    if (!m_bcamopen) {
        return false;
    }
    // TODO: grab one frame
    return true;
}

bool Balser_Cam::sanpcap()
{
    if (!m_bcamopen) {
        return false;
    }
    // TODO: continuous grab
    return true;
}

bool Balser_Cam::stopCap()
{
    return true;
}

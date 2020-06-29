#include <spacket/impl/serial_device_impl_p.h>


UARTConfig SerialDeviceImpl::config; // zero-initialized

SerialDeviceImpl* SerialDeviceImpl::instance;

typename SerialDeviceImpl::Mailbox SerialDeviceImpl::readMailbox;

thread_reference_t SerialDeviceImpl::readThreadRef;

thread_reference_t SerialDeviceImpl::writeThreadRef;

Thread SerialDeviceImpl::readThread;

typename SerialDeviceImpl::ThreadStorage SerialDeviceImpl::readThreadStorage;

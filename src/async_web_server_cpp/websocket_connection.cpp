#include "ignition/async_web_server_cpp/websocket_connection.hh"

#include <limits>

using namespace std::placeholders;

namespace async_web_server_cpp
{

WebsocketConnection::WebsocketConnection(HttpConnectionPtr connection)
  : connection_(connection) {}

void WebsocketConnection::set_message_handler(MessageHandler& handler)
{
  handler_ = handler;
}

bool WebsocketConnection::sendTextMessage(const std::string& content)
{
  async_web_server_cpp::WebsocketMessage m;
  m.type = async_web_server_cpp::WebsocketMessage::type_text;
  m.content = content;
  return sendMessage(m);
}
bool WebsocketConnection::sendPingMessage(const std::string& content)
{
  async_web_server_cpp::WebsocketMessage m;
  m.type = async_web_server_cpp::WebsocketMessage::type_ping;
  m.content = content;
  return sendMessage(m);
}


bool WebsocketConnection::sendMessage(const WebsocketMessage& message)
{
  WebsocketFrame frame;
  if (frame.fromMessage(message))
  {
    return sendFrame(frame);
  }
  return false;
}

bool WebsocketConnection::sendFrame(WebsocketFrame& frame)
{
  std::vector<unsigned char> buffer;
  if (frame.serialize(buffer))
  {
    connection_->write_and_clear(buffer);
    return true;
  }
  return false;
}


void WebsocketConnection::static_handle_read(WebsocketConnection* _this, const char* begin, const char* end)
{
  if (_this)
    _this->handle_read(begin, end);
}

void WebsocketConnection::handle_read(const char* begin, const char* end)
{
  std::optional<bool> frame_result;
  const char* parse_end = begin;

  while (parse_end < end)
  {
    std::tie(frame_result, parse_end) = frame_parser_.parse(frame_, parse_end, end);

    if (frame_result.has_value() && frame_result)
    {
      frame_parser_.reset();
      auto message_result = frame_buffer_.consume(message_, frame_);

      if (message_result.has_value() && message_result)
      {
        if (handler_)
          handler_(message_);
      }
    }
    else if (frame_result.has_value() && !frame_result)
    {
      frame_parser_.reset();
      message_.type = WebsocketMessage::type_unknown;
    }
  }
  auto weak_this = this->weak_from_this();
  connection_->async_read(std::bind(&WebsocketConnection::static_handle_read, this, _1, _2));
}

}

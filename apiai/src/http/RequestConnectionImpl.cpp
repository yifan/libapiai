#include "RequestConnectionImpl.h"

#include <apiai/exceptions/Exception.h>
#include <apiai/exceptions/InvalidArgumentException.h>

using namespace std;
using namespace ai;

RequestConnection::RequestConnectionImpl::RequestConnectionImpl(const string &URL)
{
    curl = curl_easy_init();
    if (!curl) {
        throw Exception("Cannot init CURL object.");
    }

    curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
}

size_t RequestConnection::RequestConnectionImpl::read_callback(char *ptr, size_t size, size_t nmemb, io::StreamReader *reader)
{
    return reader->read(ptr, size * nmemb);
}

uint RequestConnection::RequestConnectionImpl::write_callback(char *in, uint size, uint nmemb, string *response)
{
  response->append(in, size * nmemb);
  return size * nmemb;
}

const string &RequestConnection::RequestConnectionImpl::getURL() const
{
    return URL;
}

void RequestConnection::RequestConnectionImpl::setURL(const string &value)
{
    URL = value;
    curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
}

string RequestConnection::RequestConnectionImpl::getBody()
{
    return this->bodyStream.str();
}

void RequestConnection::RequestConnectionImpl::setBody(const string &value)
{
    this->bodyStream.str(value);
    this->bodyStream.sealed(true);
}

io::StreamWriter RequestConnection::RequestConnectionImpl::getBodyStreamWriter() {
    return this->bodyStream;
}

const map<string, string> &RequestConnection::RequestConnectionImpl::getHeaders() const
{
    return headers;
}

void RequestConnection::RequestConnectionImpl::setHeaders(const map<string, string> &value)
{
    headers = value;
}

RequestConnection::RequestConnectionImpl& RequestConnection::RequestConnectionImpl::addHeader(const string &name, const string &value) {
    headers[name] = value;

    return *this;
}

string RequestConnection::RequestConnectionImpl::performConnection()
{
    //FIXME: bodyStreamReader should be used only when POST request type
    io::StreamReader bodyStreamReader(this->bodyStream);

    if (this->getBody().length() > 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        curl_easy_setopt(curl, CURLOPT_READDATA, &bodyStreamReader);
    }

    struct curl_slist *curl_headers = NULL;

    for (auto &key_value :headers) {
        ostringstream header;
        header << key_value.first << ": " << key_value.second;

        curl_headers = curl_slist_append(curl_headers, header.str().c_str());
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);

    auto response = string();

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);


    CURLcode perform_result = curl_easy_perform(curl);

    curl_slist_free_all(curl_headers);

    if (perform_result != CURLE_OK) {
        throw ai::InvalidArgumentException("Failure perform request");
    }

    return response;
}

RequestConnection::RequestConnectionImpl::~RequestConnectionImpl()
{
    curl_easy_cleanup(curl);
}

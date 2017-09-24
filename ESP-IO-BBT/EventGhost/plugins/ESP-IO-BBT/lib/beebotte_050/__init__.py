"""Python library for interfacing with Beebotte.

Contains methods for sending persistent and transient messages and for reading data.
"""

__version__ = '0.5.0'

import json
import time
import hmac
import base64
import hashlib
import requests
from email import utils
try: import urllib.parse as urllib
except ImportError: import urllib

__publicReadEndpoint__  = "/v1/public/data/read"
__readEndpoint__        = "/v1/data/read"
__writeEndpoint__       = "/v1/data/write"
__publishEndpoint__     = "/v1/data/publish"
__connectionsEndpoint__ = "/v1/connections"
__channelsEndpoint__    = "/v1/channels"

class BBT_Types:

  """
  Beebotte resource types
  """
  ### Basic types
  Any        = "any"
  Number     = "number"
  String     = "string"
  Boolean    = "boolean"
  Object     = "object"
  Function   = "function"
  Array      = "array"

  ### Constrained types
  Alpha      = "alphabetic"
  Alphanum   = "alphanumeric"
  Decimal    = "decimal"
  Rate       = "rate"
  Percentage = "percentage"
  Email      = "email"
  GPS        = "gps"
  CPU        = "cpu"
  Memory     = "memory"
  NetIf      = "netif"
  Disk       = "disk"

  ### Unit types (all numeric - functional)
  Temp       = "temperature"
  Humidity   = "humidity"
  BodyTemp   = "body_temp"

class BBT:
  akey     = None
  skey     = None
  token    = None
  hostname = None
  port     = None
  ssl      = None


  """
  Creates and maintains an object holding user credentials and connection parameters to be used as needed.

  @param akey: The user's API key (access key) to be passed along the authentication parameters.
  @param skey: The user's secret key to be used to sign API calls.
  @param hostname: The host name where the API is implemented.
  @param port: The port number.
  @param ssl: Indicates if SSL (TLS) should be used.
  """
  def __init__(self, akey = None, skey = None, token = None, hostname = "api.beebotte.com", port = "80", ssl = False):
    if akey and skey and token == None :
      self.akey     = akey
      self.skey     = skey
    elif akey == None and skey == None and token:
      self.token    = token
    else:
      raise InitializationError("Initialization Error; Message: You must specify either (1) your access and secret keys pair or (2) your channel token!")

    self.hostname = hostname
    self.port     = port
    self.ssl      = ssl


  """
  Utility function that signs the given string with the secret key using SHA1 HMAC and returns a string containing
  the access key followed by column followed by the hash in base64.

  @param stringToSign: The string data to sign.

  @return: returns a string containing the access key followed by column followed by the SHA1 HMAC of the string to sign using the secret key encoded in base64.
    This signature format is used to authenticate API calls
  """
  def sign(self, stringToSign):
    signature = hmac.new(self.skey.encode(), stringToSign.encode(), hashlib.sha1)
    return "%s:%s" % (self.akey, bytes.decode(base64.b64encode(signature.digest())))

  """
  Creates a signature of an API call to authenticate the user and verify message integrity.

  @param verb: The HTTP verb (method) in upper case.
  @param uri: the API endpoint containing the query parameters.
  @param date: The date on the caller side.
  @param c_type: The Content type header, should be application/json.
  @param c_md5: The content MD5 hash of the data to send (should be set for POST requests)

  @return: returns the signature of the API call to be added as authorization header in the request to send.
  """
  def __signRequest__(self, verb, uri, date, c_type, c_md5 = ""):
    stringToSign = "%s\n%s\n%s\n%s\n%s" % (verb, c_md5, c_type, date, uri)
    return self.sign(stringToSign)

  """
  Checks if the given response data is OK or if an error occurred

  @param response: The response containing the response status and data

  @return: The response data in JSON if the status is OK, raises an exception otherwise (with the status code and error code)
  """
  def __processResponse__(self, response):
    code    = response['status']
    data   = {}
    if response['data']:
      try:
        data = json.loads(response['data'])
      except ValueError:
        data = response['data']
    if code < 400:
      return data
    else:
      errcode = data['error']['code']
      errmsg  = data['error']['message']
      if code == 400:
        if errcode == 1101:
          raise AuthenticationError("Status: 400; Code 1101; Message: %s" % errmsg)
        elif errcode == 1401:
          raise ParameterError("Status: 400; Code 1401; Message: %s" % errmsg)
        elif errcode == 1403:
          raise BadRequestError("Status: 400; Code 1403; Message: %s" % errmsg)
        elif errcode == 1404:
          raise TypeError("Status: 400; Code 1404; Message: %s" % errmsg)
        elif errcode == 1405:
          raise BadTypeError("Status: 400; Code 1405; Message: %s" % errmsg)
        elif errcode == 1406:
          raise PayloadLimitError("Status: 400; Code 1406; Message: %s" % errmsg)
        else:
          raise UnexpectedError("Status: %s; Code %s; Message: %s" % (code, errcode, errmsg) )
      elif code == 405:
        if errcode == 1102:
          raise NotAllowedError("Status: 405; Code 1102; Message: %s" % errmsg)
        else:
          raise UnexpectedError("Status: %s; Code %s; Message: %s" % (code, errcode, errmsg) )
      elif code == 500:
        if errcode == 1201:
          raise InternalError("Status: 500; Code 1201; Message: %s" % errmsg)
        else:
          raise InternalError("Status: %s; Code %s; Message: %s" % (code, errcode, errmsg) )
      elif code == 404:
        if errcode == 1301:
          raise NotFoundError("Status: 404; Code 1301; Message: %s" % errmsg)
        if errcode == 1302:
          raise NotFoundError("Status: 404; Code 1302; Message: %s" % errmsg)
        if errcode == 1303:
          raise NotFoundError("Status: 404; Code 1303; Message: %s" % errmsg)
        if errcode == 1304:
          raise AlreadyExistError("Status: 404; Code 1304; Message: %s" % errmsg)
        if errcode == 1305:
          raise AlreadyExistError("Status: 404; Code 1305; Message: %s" % errmsg)
        if errcode == 1306:
          raise AlreadyExistError("Status: 404; Code 1306; Message: %s" % errmsg)
        else:
          raise UnexpectedError("Status: %s; Code %s; Message: %s" % (code, errcode, errmsg) )
      else:
        raise UnexpectedError("Status: %s; Code %s; Message: %s" % (code, errcode, errmsg) )

  """
  Sends a POST request with the given data to the given URI endpoint and returns the response data.

  @param uri: The uri endpoint.
  @param data: the data to send.
  @param auth: Indicates if the request should be authenticated (defaults to true).

  @return: The response data in JSON format if success, raises an error or failure.
  """
  def __postData__(self, uri, data, auth = True):
    if self.ssl:
      url = "%s://%s:%s%s" % ( 'https', self.hostname, self.port, uri )
    else:
      url = "%s://%s:%s%s" % ( 'http', self.hostname, self.port, uri )

    if self.token:
      if auth:
        headers = { 'Content-Type': 'application/json', 'X-Auth-Token': self.token }
      else:
        headers = { 'Content-Type': 'application/json' }
    else:
      md5 = bytes.decode( base64.b64encode( hashlib.md5( str.encode( data ) ).digest() ) )
      date = utils.formatdate()
      if auth:
        sig = self.__signRequest__('POST', uri, date, "application/json", md5)
        headers = { 'Content-MD5': md5, 'Content-Type': 'application/json', 'Date': date, 'Authorization': sig }
      else:
        headers = { 'Content-MD5': md5, 'Content-Type': 'application/json', 'Date': date }

    r = requests.post( url, data=data, headers=headers )
    return self.__processResponse__( { 'status': r.status_code, 'data': r.text } )

  """
  Sends a GET request with the given query parameters to the given URI endpoint and returns the response data.

  @param uri: The uri endpoint.
  @param query: the query parameters in JSON format.
  @param auth: Indicates if the request should be authenticated (defaults to true).

  @return: The response data in JSON format if success, raises an error or failure.
  """
  def __getData__(self, uri, query, auth = True):
    if self.ssl:
      url = "%s://%s:%s%s" % ( 'https', self.hostname, self.port, uri )
    else:
      url = "%s://%s:%s%s" % ( 'http', self.hostname, self.port, uri )

    full_uri = "%s" % ( uri )
    if query != None:
      full_uri = "%s?%s" % ( full_uri, urllib.urlencode( query ) )

    if self.token:
      if auth:
        headers = { 'Content-Type': 'application/json', 'X-Auth-Token': self.token }
      else:
        headers = { 'Content-Type': 'application/json' }
    else:
      date = utils.formatdate()
      if auth:
        sig = self.__signRequest__('GET', full_uri, date, "application/json")
        headers = { 'Content-Type': 'application/json', 'Date': date, 'Authorization': sig }
      else:
        headers = { 'Content-Type': 'application/json', 'Date': date }

    r = requests.get( url, params=query, headers=headers )
    return self.__processResponse__( { 'status': r.status_code, 'data': r.text } )

  """
  Sends a DELETE request with the given query parameters to the given URI endpoint and returns the response data.

  @param uri: The uri endpoint.
  @param query: the query parameters in JSON format.
  @param auth: Indicates if the request should be authenticated (defaults to true).

  @return: The response data in JSON format if success, raises an error or failure.
  """
  def __deleteData__(self, uri, query, auth = True):
    if self.ssl:
      url = "%s://%s:%s%s" % ( 'https', self.hostname, self.port, uri )
    else:
      url = "%s://%s:%s%s" % ( 'http', self.hostname, self.port, uri )

    full_uri = "%s" % ( uri )
    if query != None:
      full_uri = "%s?%s" % ( full_uri, urllib.urlencode( query ) )

    if self.token:
      if auth:
        headers = { 'Content-Type': 'application/json', 'X-Auth-Token': self.token }
      else:
        headers = { 'Content-Type': 'application/json' }
    else:
      date = utils.formatdate()
      if auth:
        sig = self.__signRequest__('DELETE', full_uri, date, "application/json")
        headers = { 'Content-Type': 'application/json', 'Date': date, 'Authorization': sig }
      else:
        headers = { 'Content-Type': 'application/json', 'Date': date }

    r = requests.delete( url, params=query, headers=headers )
    return self.__processResponse__( { 'status': r.status_code, 'data': r.text } )

  """
  Public Read
  Reads data from the resource with the given metadata. This method expects the resource to have public access.
  In Beebotte, resources follow a 2 level hierarchy: Channel -> Resource
  Data is always associated with a Resources.
  This call will not be signed (no authentication required) as the resource is public.

  @param owner: required the owner (username) of the resource to read from.
  @param channel: required the channel name.
  @param resource: required the resource name to read from.
  @param limit: optional number of records to return.
  @param source: optional indicates whether to read from database or from historical statistics. Accepts ('raw', 'hour-stats', 'day-stats').
  @param time_range: optional indicates the timerange for the returned records. Accepts ('Xhour', 'Xday', 'Xweek', 'Xmonth') with X a positive integer, or ('today', 'yesterday', 'current-ween', 'last-week', 'current-month', 'last-month', 'ytd' ).
  @param data_filter filters data records to be returned
  @param sample_rate reduces the number of records to return (number between 0 and 1)

  @return: The response data in JSON format if success, raises an error or failure.
  """
  def readPublic(self, owner, channel, resource, limit = 750, source = "raw", time_range = None, data_filter = None, sample_rate = None):
    query = {'limit': limit, 'source': source}
    if time_range:
      query['time-range'] = time_range
    if data_filter:
      query['filter'] = data_filter
    if sample_rate:
      query['sample-rate'] = sample-rate
    endpoint = "%s/%s/%s/%s" % ( __publicReadEndpoint__, owner, channel, resource )
    response = self.__getData__( endpoint, query, False )
    return response;

  """
  Read
  Reads data from the resource with the given metadata.
  In Beebotte, resources follow a 2 level hierarchy: Channel -> Resource
  Data is always associated with Resources.
  This call will be signed to authenticate the calling user.

  @param channel: required the channel name.
  @param resource: required the resource name to read from.
  @param limit: optional number of records to return.
  @param source: optional indicates whether to read from database or from historical statistics. Accepts ('raw', 'hour-stats', 'day-stats').
  @param time_range: optional indicates the timerange for the returned records. Accepts ('Xhour', 'Xday', 'Xweek', 'Xmonth') with X a positive integer, or ('today', 'yesterday', 'current-ween', 'last-week', 'current-month', 'last-month', 'ytd' ).
  @param data_filter filters data records to be returned
  @param sample_rate reduces the number of records to return (number between 0 and 1)

  @return: The response data in JSON format if success, raises an error or failure.
  """
  def read(self, channel, resource, limit = 1, source = "raw", time_range = None, data_filter = None, sample_rate = None ):
    query = {'limit': limit, 'source': source}
    if time_range:
      query['time-range'] = time_range
    if data_filter:
      query['filter'] = data_filter
    if sample_rate:
      query['sample-rate'] = sample-rate

    endpoint = "%s/%s/%s" % ( __readEndpoint__, channel, resource )
    response = self.__getData__( endpoint, query, True )
    return response;

  """
  Write (Persistent messages)
  Writes data to the resource with the given metadata.
  In Beebotte, resources follow a 2 level hierarchy: Channel -> Resource
  Data is always associated with Resources.
  This call will be signed to authenticate the calling user.

  @param channel: required the channel name.
  @param resource: required the resource name to read from.
  @param data: required the value to write (persist).
  @param ts: optional timestamp in milliseconds (since epoch). If this parameter is not given, it will be automatically added with a value equal to the local system time. If it is set to zero, it will be ignored. In this case, the message timestamp would be Beebotte's reception time.

  @return: true on success, raises an error or failure.
  """
  def write(self, channel, resource, data, ts = None ):
    body = { 'data': data }
    if ts != None:
      body['ts'] = ts
    else:
      body['ts'] = round(time.time() * 1000)

    endpoint = "%s/%s/%s" % ( __writeEndpoint__, channel, resource )
    response = self.__postData__( endpoint, json.dumps(body, separators=(',', ':')), True )
    return response;

  """
  Bulk Write (Persistent messages)
  Writes an array of data in one API call.
  In Beebotte, resources follow a 2 level hierarchy: Channel -> Resource
  Data is always associated with Resources.
  This call will be signed to authenticate the calling user.

  @param channel: required the channel name.
  @param data_array: required the data array to send. Should follow the following format
    [{
      resource required the resource name to read from.
      data required the value to write (persist).
      ts optional timestamp in milliseconds (since epoch). If this parameter is not given, it will be automatically added with a value equal to the local system time.
    }]

  @return: true on success, raises an error or failure.
  """
  def writeBulk(self, channel, data_array ):
    body = { 'records': data_array }

    endpoint = "%s/%s" % ( __writeEndpoint__, channel)
    response = self.__postData__( endpoint, json.dumps(body, separators=(',', ':')), True )
    return response;

  """
  Publish (Transient messages)
  Publishes data to the resource with the given metadata. The published data will not be persisted. It will only be delivered to connected subscribers.
  In Beebotte, resources follow a 2 level hierarchy: Channel -> Resource
  Data is always associated with Resources.
  Publish operations does not require a resource to be initially configured on Beebotte.
  This call will be signed to authenticate the calling user.

  @param channel: required the channel name.
  @param resource: required the resource name to read from.
  @param data: required the data to publish (transient).
  @param ts: optional timestamp in milliseconds (since epoch). If this parameter is not given, it will be automatically added with a value equal to the local system time.  If it is set to zero, it will be ignored. In this case, the message timestamp would be Beebotte's reception time
  @param source: optional additional data that will be appended to the published message. This can be a logical identifier (session id) of the originator. Use this as suits you.

  @return: true on success, raises an error or failure.
  """
  def publish(self, channel, resource, data, ts = None, source = None ):
    body = { 'data': data }
    if source:
      body['source'] = source

    if ts != None:
      body['ts'] = ts
    else:
      body['ts'] = round(time.time() * 1000)

    endpoint = "%s/%s/%s" % ( __publishEndpoint__, channel, resource )
    response = self.__postData__( endpoint, json.dumps(body, separators=(',', ':')), True )
    return response;

  """
  Bulk Publish (Transient messages)
  Publishes an array of data in one API call.
  In Beebotte, resources follow a 2 level hierarchy: Channel -> Resource
  Data is always associated with Resources.
  Publish operations does not require a resource to be initially configured on Beebotte.
  This call will be signed to authenticate the calling user.

  @param channel: required the channel name.
  @param data_array: required the data array to send. Should follow the following format
    [{
      resource required the resource name to read from.
      data required the value to write (persist).
      ts optional timestamp in milliseconds (since epoch). If this parameter is not given, it will be automatically added with a value equal to the local system time.
      source: optional additional data that will be appended to the published message. This can be a logical identifier (session id) of the originator. Use this as suits you.
    }]

  @return: true on success, raises an error or failure.
  """
  def publishBulk(self, channel, data_array ):
    body = { 'records': data_array }

    endpoint = "%s/%s" % ( __publishEndpoint__, channel)
    response = self.__postData__( endpoint, json.dumps(body, separators=(',', ':')), True )
    return response;

  """
  Client Authentication (used for the Presence and Resource subscription process)
  Signs the given subscribe metadata and returns the signature.

  @param sid: required the session id of the client.
  @param channel: required the channel name. Should start with 'presence:' for presence channels and starts with 'private:' for private channels.
  @param resource: optional the resource name to read from.
  @param ttl: optional the number of seconds the signature should be considered as valid (currently ignored) for future use.
  @param read: optional indicates if read access is requested.
  @param write: optional indicates if write access is requested.

  @return: JSON object containing 'auth' element with value equal to the generated signature.
  """
  def auth_client( self, sid, channel, resource = '*', ttl = 0, read = False, write = False ):
    r = 'false'
    w = 'false'
    if read:
      r = 'true'
    if write:
      w = 'true'
    stringToSign = "%s:%s.%s.:ttl=%s:read=%s:write=%s" % ( sid, channel, resource, ttl, r, w )
    return self.sign(stringToSign)

  """
  USER CONNECTION MANAGEMENT ROUTINES
  """

  """
  Gets an Array of currently connected users (using the Websockets interface) with the given userid and session id.
  If no parameter is given, this method returns an array of all connected users.
  If @param userid is given, only connection with that user id will be given.
  If @param sid is also given, only connection with that userid and sid will be given.

  @param userid: optional the user identifier as given when connecting to the Websockets interface.
  @param sid: optional the session identifier of a Websockets connection.

  @return: Array of user connection description in JSON format on success, raises an error on failure.
  """
  def getUserConnections(self, userid = None, sid = None):
    if self.skey == None:
      raise AuthenticationError("Authentication Error; Message: This method requires access key and secret key authentication!")

    endpoint = __connectionsEndpoint__
    query = None
    if userid != None:
      endpoint = "%s/%s" % ( __connectionsEndpoint__, userid )
    elif sid != None:
      query = {'sid': sid}

    response = self.__getData__( endpoint, query, True )
    return response;

  """
  Drops Websockets connection with the given userid and session id.
  If @param sid is is not provided, all connections with @param userid will be dropped.

  @param userid: required the user identifier as given when connecting to the Websockets interface.
  @param sid: optional the session identifier of a Websockets connection.

  @return: OK on success, raises an error on failure.
  """
  def dropUserConnection(self, userid, sid = None):
    if self.skey == None:
      raise AuthenticationError("Authentication Error; Message: This method requires access key and secret key authentication!")

    query = None
    if sid != None:
      query = {'sid': sid}

    endpoint = "%s/drop/%s" % ( __connectionsEndpoint__, userid )
    response = self.__deleteData__( endpoint, query, True )
    return response;

  """
  CHANNEL AND RESOURCE MANAGEMENT ROUTINES
  """

  """
  Adds a new resource to the given channel.

  @param channel: required the channel name.
  @param name: the resource name.
  @param vtype: the resource value type; defaults to Any type.
  @param label: optional the resource label.
  @param description: optional the resource description.
  @param sendOnSubscribe: optional indicates if most recent value should be sent on subscription.defaults to false.

  @return True os success, raises an error on failure.
  """
  def addResource(
    self,
    channel,
    name,
    vtype = BBT_Types.Any,
    label = None,
    description = None,
    sendOnSubscribe = False
  ):

    if self.skey == None:
      raise AuthenticationError("Authentication Error; Message: This method requires access key and secret key authentication!")

    endpoint = "%s/%s/resources" % ( __channelsEndpoint__, channel )

    _label = label or name
    _desc = description or _label

    body = {
      'name': name,
      'label': _label,
      'description': _desc,
      'vtype': vtype,
      'sendOnSubscribe': sendOnSubscribe
    }

    response = self.__postData__( endpoint, json.dumps(body, separators=(',', ':')), True )
    return response;


  """
  Returns a resource or all resources of a given channel.
  If resource is given, only the specified resource description will be returned, otherwise, an array of
  all the resource descriptions of that channel will be returned.

  @param channel: required the channel name.
  @param resource: optional the resource name.
  @return Array of resurce descriptions on success, raises an error on failure.
  """
  def getResource(self, channel, resource = None):
    if self.skey == None:
      raise AuthenticationError("Authentication Error; Message: This method requires access key and secret key authentication!")

    endpoint = "%s/%s/resources" % ( __channelsEndpoint__, channel )

    if resource != None:
      endpoint = "%s/%s" % ( endpoint, resource )

    response = self.__getData__( endpoint, None, True )
    return response;

  """
  Deletes a resource from a channel.

  @param channel: required the channel name.
  @param resource: required the resource name to read from.

  @return true on success, raises an error on failure.
  """
  def deleteResource(self, channel, resource):
    if self.skey == None:
      raise AuthenticationError("Authentication Error; Message: This method requires access key and secret key authentication!")

    endpoint = "%s/%s/resources/%s" % ( __channelsEndpoint__, channel, resource )
    response = self.__deleteData__( endpoint, None, True )
    return response;


  """
  Adds a new channel.

  @param name: the channel name.
  @param label: optional the channel label.
  @param description: optional the channel description.
  @param ispublic: optional indicates if the channel is public; defaults to False.
  @param resources: optional an array of resources in the channel.

  @return True os success, raises an error on failure.
  """
  def addChannel(self, name, label = None, description = None, ispublic = False, resources = [] ):

    if self.skey == None:
      raise AuthenticationError("Authentication Error; Message: This method requires access key and secret key authentication!")

    _label = label or name
    _desc = description or _label

    body = {
      'name': name,
      'label': _label,
      'description': _desc,
      'ispublic': ispublic,
      'resources': resources
    }

    endpoint = "%s" % ( __channelsEndpoint__ )
    response = self.__postData__( endpoint, json.dumps(body, separators=(',', ':')), True )
    return response;


  """
  Returns a description of the given channel or of all the user account's channels if channel is not provided.

  @param channel: optional the channel name.

  @return Array of channel descriptions on success, raises an error on failure.
  """
  def getChannel(self, channel = None):
    if self.skey == None:
      raise AuthenticationError("Authentication Error; Message: This method requires access key and secret key authentication!")

    endpoint = __channelsEndpoint__

    if channel != None:
      endpoint = "%s/%s" % ( endpoint, channel )

    response = self.__getData__( endpoint, None, True )
    return response;

  """
  Deletes the specified channel from the user account's.

  @param channel: required the channel name to delete.

  @return true on success, raises an error on failure.
  """
  def deleteChannel(self, channel):
    if self.skey == None:
      raise AuthenticationError("Authentication Error; Message: This method requires access key and secret key authentication!")

    endpoint = "%s/%s" % ( __channelsEndpoint__, channel )
    response = self.__deleteData__( endpoint, None, True )
    return response;

  """
  Regenerates a new Channel Token for the given @channel if it exists.
  Regenerating a new Channel Token will invalidate the last active token. A channel can only have one Token at a time.

  @param channel: required the channel name.

  @return JSON description of the channel with the newly generated Token on success, raises an error on failure.
  """
  def regenerateChannelToken(self, channel):
    if self.skey == None:
      raise AuthenticationError("Authentication Error; Message: This method requires access key and secret key authentication!")

    endpoint = "%s/%s/token/regenerate" % ( __channelsEndpoint__, channel )

    response = self.__getData__( endpoint, None, True )
    return response;

"""
Utility class for dealing with Resources
Contains methods for sending persistent and transient messages and for reading data.
Mainly wrappers around Beebotte API calls.
"""
class Resource:
  channel   = None
  resource = None
  bbt      = None

  """
  Constructor, initializes the Resource object.
  In Beebotte, resources follow a 2 level hierarchy: Channel -> Resource
  Data is always associated with Resources.

  @param bbt: required reference to the Beebotte client connector.
  @param channel: required channel name.
  @param resource: required resource name.
  """
  def __init__(self, bbt, channel, resource):
    self.channel   = channel
    self.resource = resource
    self.bbt      = bbt

  """
  Write (Persistent messages)
  Writes data to this resource.
  This call will be signed to authenticate the calling user.

  @param data: required the value to write (persist).
  @param ts: optional timestamp in milliseconds (since epoch). If this parameter is not given, it will be automatically added with a value equal to the local system time.

  @return: true on success, raises an error or failure.
  """
  def write(self, data, ts = None):
    return self.bbt.write(self.channel, self.resource, ts = ts, data = data)

  """
  Publish (Transient messages)
  Publishes data to this resource. The published data will not be persisted. It will only be delivered to connected subscribers.
  This call will be signed to authenticate the calling user.

  @param data: required the data to publish (transient).
  @param ts: optional timestamp in milliseconds (since epoch). If this parameter is not given, it will be automatically added with a value equal to the local system time.

  @return: true on success, raises an error or failure.
  """
  def publish(self, data, ts = None):
    return self.bbt.publish(self.channel, self.resource, ts = ts, data = data)

  """
  Read
  Reads data from the this resource.
  If the owner is set (value different than None) the behaviour is Public Read (no authentication).
  If the owner is None, the behaviour is authenticated read.

  @param limit: optional number of records to return.
  @param owner: optional the owner (username) of the resource to read from for public read. None to read from the user's owned channel.
  @param source: optional indicates whether to read from database or from historical statistics. Accepts ('raw', 'hour-stats', 'day-stats').
  @param time_range: optional indicates the timerange for the returned records. Accepts ('Xhour', 'Xday', 'Xweek', 'Xmonth') with X a positive integer, or ('today', 'yesterday', 'current-ween', 'last-week', 'current-month', 'last-month', 'ytd' ).
  @param data_filter filters data records to be returned
  @param sample_rate reduces the number of records to return (number between 0 and 1)

  @return: array of records (JSON) on success, raises an error or failure.
  """
  def read(self, limit = 750, owner = None, source = "raw", time_range = None, data_filter = None, sample_rate = None):
    if owner != None:
      return self.bbt.publicRead( owner, self.channel, self.resource, limit, source, time_range, data_filter, sample_rate )
    else:
      return self.bbt.read(channel=self.channel, resource=self.resource, limit=limit, source=source, time_range=time_range)

  """
  Read
  Reads the last inserted record.

  @return: the last inserted record on success, raises an error or failure.
  """
  def recentVal(self):
    return self.bbt.read(self.channel, self.resource, limit = 1)[0]

class DataPoint:
  channel  = None
  resource = None
  data     = None
  ts       = None

  def __init__(self, data, ts, channel = None, resource = None):
    self.channel  = channel
    self.resource = resource
    self.data     = data
    self.ts       = ts

  @classmethod
  def fromJSON(cls, params):
    #return cls( params['channel'], params['resource'], params['data'], params['ts'] )
    return cls( data = params['data'], ts = params['ts'] )

  def toJSON(self, owner = None):
    return {'owner': owner, 'channel': self.channel, 'resource': self.resource, 'data': self.data, 'ts': self.ts}

class InitializationError(Exception):
    pass

class AuthenticationError(Exception):
    pass

class InternalError(Exception):
    pass

class NotFoundError(Exception):
    pass

class AlreadyExistError(Exception):
    pass

class NotAllowedError(Exception):
    pass

class UsageLimitError(Exception):
    pass

class PayloadLimitError(Exception):
    pass

class UnexpectedError(Exception):
    pass

class BadRequestError(Exception):
    pass

class ParameterError(Exception):
    pass

class TypeError(Exception):
    pass

class BadTypeError(Exception):
    pass


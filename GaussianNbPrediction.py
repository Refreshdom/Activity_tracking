import paho.mqtt.client as mqtt
from json import loads, dumps
from datetime import datetime
from pathlib2 import Path
import time
import httplib, urllib

# Load libraries
import pandas
from pandas.tools.plotting import scatter_matrix
import matplotlib.pyplot as plt
from sklearn import model_selection
from sklearn.naive_bayes import GaussianNB

#-----User-defined parameters-----#
broker = "broker.hivemq.com" 
port = 1883

sub_topics = [("linkit/sensor/mpu", 1)]

now=time.time()

#apikey = "KQC2GE05YYSB9RK0"

value=0

# Load dataset
path = "/home/pi/homeml/dataset.csv"
names = ['ax', 'ay', 'az', 'gx', 'gy', 'gz', 'class']
dataset = pandas.read_csv(path, names=names)

# shape
print(dataset.shape)
print(dataset.head(20))

# descriptions
print(dataset.describe())

# class distribution
print(dataset.groupby('class').size())

# Split-out validation dataset
array = dataset.values
X = array[:,0:6]
Y = array[:,6]
validation_size = 0.20
seed = 10
X_train, X_validation, Y_train, Y_validation = model_selection.train_test_split(X, Y, test_size=validation_size, random_state=seed)

scoring = 'accuracy'

models = []
models.append(('NB', GaussianNB()))

# evaluate each model in turn
results = []
names = []

tmp_str = [0,0,0,0,0,0]

for name, model in models:
	kfold = model_selection.KFold(n_splits=10, random_state=seed)
	cv_results = model_selection.cross_val_score(model, X_train, Y_train, cv=kfold, scoring=scoring)
	results.append(cv_results)
	names.append(name)
	msg = "%s: %f (%f)" % (name, cv_results.mean(), cv_results.std())
	print(msg)

gnb = GaussianNB()
gnb.fit(X_train, Y_train)
	
    
def on_connect(client1, userdata, flags, rc):
    print("rc: " + str(rc))
    client1.subscribe(sub_topics)

def on_subscribe(client1, obj, mid, granted_qos):
    print("Subscribed: "+str(mid)+" "+str(granted_qos))

def on_message(client1, userdata, msg):
    global tmp_str
    tmp_str[:] = []
    scanner_data = loads(msg.payload)    # decode JSON message
    dt = datetime.now()
    tmp_str.append(scanner_data['ax'])
    tmp_str.append(scanner_data['ay'])
    tmp_str.append(scanner_data['az'])
    tmp_str.append(scanner_data['gx'])
    tmp_str.append(scanner_data['gy'])
    tmp_str.append(scanner_data['gz'])
	
	## To combine activity tracking and locationing system
    # print(tmp_str)
    # for b in scanner_data['mpu_data']:
        # tmp_str = (tmp_str + str(dt.date()) + ',' + str(dt.time()) + ',' + scanner_data['counter'] + ',' +
                  # scanner_data['scanner'] + ',' + b + ',' + str(scanner_data['beacons'][b]) + '\n')


def ActivitySent(pActivity): 
	#Function> post incoming predicted activity to ThinkSpeak.com
    global value

    if(pActivity == 'lying'): 
        value=0        
    if(pActivity == 'sitting'):
        value=1                
    if(pActivity == 'standing'):
        value=2  
    if(pActivity == 'walking'):   
        value=3  
    if(pActivity == 'running'):		    
        value=4  
    print(value)
    params = "/update?api_key="+apikey+"&field1=0&field2=0&field3=1000&field4=1&field6="+str(value)
    headers = {"Content-typ": "application/x-www-form-urlencoded", "Accept": "text/plain"}
    conn = httplib.HTTPConnection("iotfoe.ddns.net:3333")
    conn.request("POST", params, "", headers)
    response = conn.getresponse()
    print response.status, response.reason
    data = response.read()
    conn.close()
 
client1 = mqtt.Client("mpuSub", clean_session=True)
client1.on_connect = on_connect
client1.on_message = on_message
client1.on_subscribe = on_subscribe
client1.connect(broker, port)
run=True
str_predict = []
while run:
    if(time.time()-now)>1:
	# predict every 1ms
        global tmp_str
        now=time.time()
        current_activity = []
        current_activity.append(tmp_str)
        print(current_activity)
        predictions = gnb.predict(current_activity)
        ActivitySent(predictions[0])
    client1.loop()

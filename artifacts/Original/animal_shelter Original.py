from pymongo import MongoClient
from bson.objectid import ObjectId
from bson.json_util import dumps
from ipywidgets import IntSlider
from ipywidgets.embed import embed_minimal_html
from bson.objectid import ObjectId
import json
import urllib.parse

class AnimalShelter(object):
    
    #property variables
    records_updated = 0 #keep a record of the records updated in an operation; CYA
    records_matched = 0 #keep a record of the records macthed in an operation; CYA
    records_deleted = 0 #keep a record of the records deleted in an operation; CYA

    #constructor to init the mongodb
    #to do: this should be a singleton
    def __init__(self, _password, _username):
        
        client = MongoClient('mongodb://aacuser:SNHU1234@nv-desktop-services.apporto.com:32432/?directConnection=true&appName=mongosh+1.8.0')
        animal_data = client.AAC.animals.find({})
        
        #URI must be percent escaped as per pymongo documentation
        username = urllib.parse.quote_plus(_username)
        password = urllib.parse.quote_plus(_password)
        
        self.client = MongoClient('mongodb://aacuser:SNHU1234@nv-desktop-services.apporto.com:32432/?directConnection=true&appName=mongosh+1.8.0')
        self.database = self.client['AAC']
       
    #Mehtod to create a record
    #Input data formatted as per the Pymongo API
    def createRecord(self, data):
        if data:
            _insertValid = self.dataBase.animals.insert_one(data)
            #check the status of the inserted value 
            return True if _insertValid.acknowledged else False
        else:
            raise Exception("No document to save. Data is empty.")
    
    #todo implement the R
    #get documents by the GUID
    #This is more for a test but could be used after the createRecord
    #Since the document returned by insert_one contains the newly created _id
    def getRecordId(self, postId):
        _data = self.database.find_one({'_id': ObjectId(postId)})
                                  
        return _data
    
    #Get records with criteria
    #All records are returned if criteria is None
    #Default is None
    #do not return the _id
    def getRecordCriteria(self, criteria):
        if criteria:
            _data = self.database.animals.find(criteria, {'_id' : 0})
                                 
        else:
            _data = self.database.animals.find({},{'_id' : 0})
                                  
        return _data
    
    #Update a record
    def updateRecord(self, query, newValue):
        if not query:
            raise Exception("No search criteria is present.")
        elif not newValue:
            raise Exception("No update value is present.")
        else:
            _updateValid = self.database.animals.update_many(query, {"$set": newValue})
            self.records_updated = _updateValid.modified_count
            self.records_matched = _updateValid.matched_count

            return True if _updateValid.modified_count > 0 else False
    
    #delete a record
    def deleteRecord(self, query):
        if not query:
            raise Exception("No search criteria is present.")
        
        else:
            _deleteValid = self.database.animals.delete_many(query)
            self.records_deleted = _deleteValid.deleted_count

            return True if _deleteValid.deleted_count > 0 else False                   

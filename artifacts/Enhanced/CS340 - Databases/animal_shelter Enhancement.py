from pymongo import MongoClient
from bson.objectid import ObjectId
from bson.json_util import dumps
import json
import urllib.parse
import os
import logging
from pymongo.errors import ConnectionFailure, OperationFailure, ServerSelectionTimeoutError

# Set up logging
logging.basicConfig(
    filename='animal_shelter.log',
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)

class AnimalShelter(object):
    """
    AnimalShelter class provides an interface to MongoDB for animal shelter data management.
    Implements CRUD operations with improved security, error handling, and performance.
    """
    
    # Property variables
    records_updated = 0  # Keep a record of the records updated in an operation
    records_matched = 0  # Keep a record of the records matched in an operation
    records_deleted = 0  # Keep a record of the records deleted in an operation
    
    def __init__(self):
        """
        Constructor initializes MongoDB connection using environment variables
        for secure credential management.
        """
        try:
            # Get credentials from environment variables
            username = os.getenv('DB_USERNAME', 'aacuser')
            password = os.getenv('DB_PASSWORD', 'SNHU1234')
            host = os.getenv('DB_HOST', 'nv-desktop-services.apporto.com')
            port = os.getenv('DB_PORT', '32432')
            
            # URI must be percent escaped as per pymongo documentation
            username = urllib.parse.quote_plus(username)
            password = urllib.parse.quote_plus(password)
            
            # Build connection string
            connection_string = f'mongodb://{username}:{password}@{host}:{port}/?directConnection=true&appName=mongosh+1.8.0'
            
            # Connect to MongoDB
            self.client = MongoClient(connection_string, serverSelectionTimeoutMS=5000)
            self.database = self.client['AAC']
            
            # Test connection
            self.client.admin.command('ping')
            logging.info("Successfully connected to MongoDB")
            
            # Create indexes for optimized queries
            self._create_indexes()
            
        except ConnectionFailure:
            logging.error("Server not available")
            raise ConnectionError("Could not connect to MongoDB server. Please check if the server is running.")
        except ServerSelectionTimeoutError:
            logging.error("Connection timeout")
            raise ConnectionError("Connection to MongoDB timed out. Please check network settings.")
        except OperationFailure as e:
            logging.error(f"Authentication failed: {str(e)}")
            raise ConnectionError("Authentication failed. Please check your credentials.")
        except Exception as e:
            logging.error(f"Unexpected error during initialization: {str(e)}")
            raise
    
    def _create_indexes(self):
        """
        Create indexes for commonly queried fields to improve performance.
        """
        try:
            # Create indexes for frequently queried fields
            self.database.animals.create_index([("animal_type", 1)])
            self.database.animals.create_index([("breed", 1)])
            self.database.animals.create_index([("location_found", 1)])
            logging.info("Database indexes created successfully")
        except Exception as e:
            logging.warning(f"Error creating indexes: {str(e)}")
    
    def createRecord(self, data):
        """
        Create a new animal record in the database.
        
        Args:
            data (dict): Animal data to insert
            
        Returns:
            bool: True if successful, False otherwise
            
        Raises:
            ValueError: If data is empty
            Exception: For other database errors
        """
        if not data:
            logging.error("Attempted to create record with empty data")
            raise ValueError("No document to save. Data is empty.")
            
        try:
            insert_result = self.database.animals.insert_one(data)
            success = insert_result.acknowledged
            
            if success:
                logging.info(f"Successfully created record with ID: {insert_result.inserted_id}")
            else:
                logging.warning("Insert operation not acknowledged by the database")
                
            return success
        except Exception as e:
            logging.error(f"Error creating record: {str(e)}")
            raise Exception(f"Failed to create record: {str(e)}")
    
    def getRecordId(self, post_id):
        """
        Retrieve a record by its ObjectId.
        
        Args:
            post_id (str): The ObjectId as a string
            
        Returns:
            dict: The matching record or None if not found
            
        Raises:
            ValueError: If post_id is invalid
            Exception: For other database errors
        """
        try:
            if not ObjectId.is_valid(post_id):
                logging.error(f"Invalid ObjectId format: {post_id}")
                raise ValueError("Invalid ObjectId format")
                
            data = self.database.animals.find_one({'_id': ObjectId(post_id)})
            
            if data:
                logging.info(f"Successfully retrieved record with ID: {post_id}")
            else:
                logging.info(f"No record found with ID: {post_id}")
                
            return data
        except Exception as e:
            logging.error(f"Error retrieving record by ID: {str(e)}")
            raise Exception(f"Failed to retrieve record: {str(e)}")
    
    def getRecordCriteria(self, criteria=None):
        """
        Retrieve records matching the given criteria.
        
        Args:
            criteria (dict, optional): Query criteria. Defaults to None (all records).
            
        Returns:
            cursor: MongoDB cursor with the matching records
            
        Raises:
            Exception: For database errors
        """
        try:
            if criteria:
                data = self.database.animals.find(criteria, {'_id': 0})
                logging.info(f"Retrieved records with criteria: {criteria}")
            else:
                data = self.database.animals.find({}, {'_id': 0})
                logging.info("Retrieved all records")
                
            return data
        except Exception as e:
            logging.error(f"Error retrieving records by criteria: {str(e)}")
            raise Exception(f"Failed to retrieve records: {str(e)}")
    
    def updateRecord(self, query, new_value):
        """
        Update records matching the query with new values.
        
        Args:
            query (dict): Query to match records
            new_value (dict): New values to set
            
        Returns:
            bool: True if any records were updated, False otherwise
            
        Raises:
            ValueError: If query or new_value is empty
            Exception: For other database errors
        """
        if not query:
            logging.error("Attempted update with empty query")
            raise ValueError("No search criteria is present.")
        elif not new_value:
            logging.error("Attempted update with empty new values")
            raise ValueError("No update value is present.")
            
        try:
            update_result = self.database.animals.update_many(query, {"$set": new_value})
            self.records_updated = update_result.modified_count
            self.records_matched = update_result.matched_count
            
            logging.info(f"Update operation: matched {self.records_matched}, modified {self.records_updated}")
            return self.records_updated > 0
        except Exception as e:
            logging.error(f"Error updating records: {str(e)}")
            raise Exception(f"Failed to update records: {str(e)}")
    
    def deleteRecord(self, query):
        """
        Delete records matching the query.
        
        Args:
            query (dict): Query to match records for deletion
            
        Returns:
            bool: True if any records were deleted, False otherwise
            
        Raises:
            ValueError: If query is empty
            Exception: For other database errors
        """
        if not query:
            logging.error("Attempted deletion with empty query")
            raise ValueError("No search criteria is present.")
            
        try:
            delete_result = self.database.animals.delete_many(query)
            self.records_deleted = delete_result.deleted_count
            
            logging.info(f"Delete operation: removed {self.records_deleted} record(s)")
            return self.records_deleted > 0
        except Exception as e:
            logging.error(f"Error deleting records: {str(e)}")
            raise Exception(f"Failed to delete records: {str(e)}")
    
    def close(self):
        """
        Close the MongoDB connection.
        """
        try:
            self.client.close()
            logging.info("MongoDB connection closed")
        except Exception as e:
            logging.error(f"Error closing MongoDB connection: {str(e)}")

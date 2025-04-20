import unittest
import os
from animal_shelter import AnimalShelter
from pymongo import MongoClient
import pymongo.errors

class TestAnimalShelter(unittest.TestCase):
    """
    Tests for AnimalShelter class functionality.
    Runs against a test database to keep production data safe.
    """
    
    @classmethod
    def setUpClass(cls):
        """
        Set up test environment.
        Creates a connection to the test database.
        """
        # Set environment variables for test database
        os.environ['DB_USERNAME'] = 'aacuser'
        os.environ['DB_PASSWORD'] = 'SNHU1234'
        os.environ['DB_HOST'] = 'nv-desktop-services.apporto.com'
        os.environ['DB_PORT'] = '32432'
        
        # Create a direct connection to test database for setup/teardown
        username = os.environ.get('DB_USERNAME')
        password = os.environ.get('DB_PASSWORD')
        host = os.environ.get('DB_HOST')
        port = os.environ.get('DB_PORT')
        
        cls.client = MongoClient(f'mongodb://{username}:{password}@{host}:{port}/?directConnection=true')
        cls.client.drop_database('test_animals')
        cls.test_db = cls.client['test_animals']
        
        # Create animal_shelter instance
        try:
            cls.animal_shelter = AnimalShelter()
            # Override the database to use test database
            cls.animal_shelter.database = cls.test_db
        except Exception as e:
            print(f"Error setting up test environment: {str(e)}")
            raise
    
    @classmethod
    def tearDownClass(cls):
        """
        Clean up after tests.
        Drops the test database and closes connections.
        """
        try:
            cls.client.drop_database('test_animals')
            cls.client.close()
            cls.animal_shelter.close()
        except Exception as e:
            print(f"Error tearing down test environment: {str(e)}")
    
    def setUp(self):
        """
        Set up before each test.
        Clears the animals collection.
        """
        self.test_db.animals.delete_many({})
    
    def test_create_record_success(self):
        """Test creating a record successfully."""
        test_data = {
            'animal_id': 'A123',
            'animal_type': 'Dog',
            'breed': 'Labrador',
            'name': 'Buddy',
            'age': 3
        }
        
        result = self.animal_shelter.createRecord(test_data)
        self.assertTrue(result)
        
        # Verify record was created
        count = self.test_db.animals.count_documents({'animal_id': 'A123'})
        self.assertEqual(count, 1)
    
    def test_create_record_empty_data(self):
        """Test creating a record with empty data raises ValueError."""
        with self.assertRaises(ValueError):
            self.animal_shelter.createRecord({})
    
    def test_get_record_by_id(self):
        """Test retrieving a record by ID."""
        # Insert a test record
        test_data = {'animal_id': 'B456', 'name': 'Max'}
        insert_result = self.test_db.animals.insert_one(test_data)
        
        # Retrieve the record by ID
        record = self.animal_shelter.getRecordId(str(insert_result.inserted_id))
        
        self.assertIsNotNone(record)
        self.assertEqual(record['animal_id'], 'B456')
        self.assertEqual(record['name'], 'Max')
    
    def test_get_record_by_invalid_id(self):
        """Test retrieving a record with invalid ID format raises ValueError."""
        with self.assertRaises(ValueError):
            self.animal_shelter.getRecordId('invalid-id')
    
    def test_get_record_criteria(self):
        """Test retrieving records by criteria."""
        # Insert test records
        self.test_db.animals.insert_many([
            {'animal_id': 'C789', 'animal_type': 'Cat', 'breed': 'Siamese'},
            {'animal_id': 'D012', 'animal_type': 'Dog', 'breed': 'Poodle'},
            {'animal_id': 'C345', 'animal_type': 'Cat', 'breed': 'Persian'}
        ])
        
        # Retrieve cats
        cats = list(self.animal_shelter.getRecordCriteria({'animal_type': 'Cat'}))
        
        self.assertEqual(len(cats), 2)
        self.assertIn('Siamese', [cat['breed'] for cat in cats])
        self.assertIn('Persian', [cat['breed'] for cat in cats])
    
    def test_get_all_records(self):
        """Test retrieving all records."""
        # Insert test records
        self.test_db.animals.insert_many([
            {'animal_id': 'E678', 'animal_type': 'Dog'},
            {'animal_id': 'F901', 'animal_type': 'Cat'}
        ])
        
        # Retrieve all records
        all_records = list(self.animal_shelter.getRecordCriteria())
        
        self.assertEqual(len(all_records), 2)
    
    def test_update_record(self):
        """Test updating records."""
        # Insert test records
        self.test_db.animals.insert_many([
            {'animal_id': 'G234', 'animal_type': 'Dog', 'name': 'Rex', 'age': 2},
            {'animal_id': 'H567', 'animal_type': 'Dog', 'name': 'Fido', 'age': 4}
        ])
        
        # Update age for dogs
        result = self.animal_shelter.updateRecord(
            {'animal_type': 'Dog'}, 
            {'status': 'Available'}
        )
        
        self.assertTrue(result)
        self.assertEqual(self.animal_shelter.records_updated, 2)
        self.assertEqual(self.animal_shelter.records_matched, 2)
        
        # Verify update
        available_dogs = self.test_db.animals.count_documents({
            'animal_type': 'Dog', 
            'status': 'Available'
        })
        self.assertEqual(available_dogs, 2)
    
    def test_update_record_no_match(self):
        """Test updating records with no matches."""
        # Update with criteria that won't match any records
        result = self.animal_shelter.updateRecord(
            {'animal_id': 'non-existent'}, 
            {'status': 'Adopted'}
        )
        
        self.assertFalse(result)
        self.assertEqual(self.animal_shelter.records_updated, 0)
        self.assertEqual(self.animal_shelter.records_matched, 0)
    
    def test_update_record_empty_query(self):
        """Test updating with empty query raises ValueError."""
        with self.assertRaises(ValueError):
            self.animal_shelter.updateRecord({}, {'status': 'Adopted'})
    
    def test_update_record_empty_new_value(self):
        """Test updating with empty new value raises ValueError."""
        with self.assertRaises(ValueError):
            self.animal_shelter.updateRecord({'animal_type': 'Dog'}, {})
    
    def test_delete_record(self):
        """Test deleting records."""
        # Insert test records
        self.test_db.animals.insert_many([
            {'animal_id': 'I890', 'animal_type': 'Cat', 'status': 'Adopted'},
            {'animal_id': 'J123', 'animal_type': 'Dog', 'status': 'Adopted'}
        ])
        
        # Delete adopted cats
        result = self.animal_shelter.deleteRecord({
            'animal_type': 'Cat', 
            'status': 'Adopted'
        })
        
        self.assertTrue(result)
        self.assertEqual(self.animal_shelter.records_deleted, 1)
        
        # Verify deletion
        remaining = self.test_db.animals.count_documents({})
        self.assertEqual(remaining, 1)
        
        cat_count = self.test_db.animals.count_documents({'animal_type': 'Cat'})
        self.assertEqual(cat_count, 0)
    
    def test_delete_record_no_match(self):
        """Test deleting records with no matches."""
        result = self.animal_shelter.deleteRecord({'animal_id': 'non-existent'})
        
        self.assertFalse(result)
        self.assertEqual(self.animal_shelter.records_deleted, 0)
    
    def test_delete_record_empty_query(self):
        """Test deleting with empty query raises ValueError."""
        with self.assertRaises(ValueError):
            self.animal_shelter.deleteRecord({})


if __name__ == '__main__':
    unittest.main()

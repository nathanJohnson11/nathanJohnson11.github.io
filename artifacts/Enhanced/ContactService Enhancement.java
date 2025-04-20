import java.util.HashMap;
import java.util.ArrayList;
import java.util.logging.Logger;

/**
 * ContactService class that manages contacts using a HashMap for efficient lookup
 * This is an enhanced version with improved data structure and error handling
 */
public class ContactService {
    // HashMap to store contacts with contactID as the key for O(1) lookup
    private HashMap<String, Contact> contactMap;
    private static final Logger logger = Logger.getLogger(ContactService.class.getName());
    
    /**
     * Constructor that initializes the HashMap
     */
    public ContactService() {
        contactMap = new HashMap<>();
    }
    
    /**
     * Add a new contact to the service
     * 
     * @param firstName First name of the contact
     * @param lastName Last name of the contact
     * @param phone Phone number (must be 10 digits)
     * @param address Address of the contact
     * @return The contact array with the new contact added
     */
    public Contact addContact(String firstName, String lastName, String phone, String address) {
        // Generate a new unique ID
        String contactId = generateUniqueId();
        
        try {
            // Create the contact - validation happens in the Contact constructor
            Contact newContact = new Contact(contactId, firstName, lastName, phone, address);
            
            // Store the contact in the HashMap for O(1) access
            contactMap.put(contactId, newContact);
            
            logger.info("Added new contact with ID: " + contactId);
            return newContact;
        } catch (IllegalArgumentException e) {
            logger.warning("Failed to add contact: " + e.getMessage());
            throw e;
        }
    }
    
    /**
     * Delete a contact by its ID
     * 
     * @param contactId The ID of the contact to delete
     * @return true if deletion was successful, false if contact not found
     */
    public boolean deleteContact(String contactId) {
        if (contactId == null || contactId.trim().isEmpty()) {
            logger.warning("Attempt to delete contact with null or empty ID");
            return false;
        }
        
        // O(1) lookup and removal
        Contact removedContact = contactMap.remove(contactId);
        
        if (removedContact != null) {
            logger.info("Deleted contact with ID: " + contactId);
            return true;
        } else {
            logger.warning("Contact not found for deletion: " + contactId);
            return false;
        }
    }
    
    /**
     * Update a contact by its ID, only changing fields that are provided
     * 
     * @param contactId The ID of the contact to update
     * @param firstName New first name (or null to leave unchanged)
     * @param lastName New last name (or null to leave unchanged)
     * @param phone New phone number (or null to leave unchanged)
     * @param address New address (or null to leave unchanged)
     * @return true if update was successful, false if contact not found
     */
    public boolean updateContact(String contactId, String firstName, String lastName, String phone, String address) {
        if (contactId == null || contactId.trim().isEmpty()) {
            logger.warning("Attempt to update contact with null or empty ID");
            return false;
        }
        
        // O(1) lookup
        Contact contact = contactMap.get(contactId);
        
        if (contact == null) {
            logger.warning("Contact not found for update: " + contactId);
            return false;
        }
        
        // Create a new contact with the updated fields
        // Only update fields that are provided (not null)
        try {
            String newFirstName = (firstName != null) ? firstName : contact.firstName;
            String newLastName = (lastName != null) ? lastName : contact.lastName;
            String newPhone = (phone != null) ? phone : contact.phone;
            String newAddress = (address != null) ? address : contact.address;
            
            // Create a new contact with the updated fields
            // Validation will happen in the Contact constructor
            Contact updatedContact = new Contact(contactId, newFirstName, newLastName, newPhone, newAddress);
            
            // Replace the old contact with the updated one
            contactMap.put(contactId, updatedContact);
            
            logger.info("Updated contact with ID: " + contactId);
            return true;
        } catch (IllegalArgumentException e) {
            logger.warning("Failed to update contact: " + e.getMessage());
            throw e;
        }
    }
    
    /**
     * Get a contact by its ID
     * 
     * @param contactId The ID of the contact to retrieve
     * @return The Contact object, or null if not found
     */
    public Contact getContact(String contactId) {
        if (contactId == null || contactId.trim().isEmpty()) {
            logger.warning("Attempt to get contact with null or empty ID");
            return null;
        }
        
        // O(1) lookup
        return contactMap.get(contactId);
    }
    
    /**
     * Get all contacts as an ArrayList
     * 
     * @return ArrayList containing all contacts
     */
    public ArrayList<Contact> getAllContacts() {
        return new ArrayList<>(contactMap.values());
    }
    
    /**
     * Display all contacts to the console
     */
    public void displayContactList() {
        if (contactMap.isEmpty()) {
            System.out.println("No contacts available.");
            return;
        }
        
        for (Contact contact : contactMap.values()) {
            System.out.println(contact.contactID + ", " + 
                               contact.firstName + ", " + 
                               contact.lastName + ", " + 
                               contact.phone + ", " + 
                               contact.address);
        }
    }
    
    /**
     * Count the number of contacts
     * 
     * @return The number of contacts
     */
    public int getContactCount() {
        return contactMap.size();
    }
    
    /**
     * Generate a unique ID for a new contact
     * 
     * @return A unique ID string
     */
    private String generateUniqueId() {
        // Simple implementation - you could use UUID or other strategies
        return String.valueOf(contactMap.size());
    }
}
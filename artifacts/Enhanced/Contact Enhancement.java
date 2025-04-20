/**
 * Contact class representing a single contact in the contact management system
 * Uses constructor validation to ensure data integrity
 */
public class Contact {
    String contactID; //no longer than 10 chars and can't be null
    String firstName; //no longer than 10 chars and can't be null
    String lastName; //no longer than 10 chars and can't be null
    String phone; //must be exactly 10 characters and cannot be null
    String address; //must be max 30 chars and cannot be null
    
    /**
     * Constructor to create a new Contact with validation
     * 
     * @param contactID Unique identifier for the contact
     * @param firstName First name of the contact
     * @param lastName Last name of the contact
     * @param phone Phone number of the contact
     * @param address Address of the contact
     * @throws IllegalArgumentException if any validation fails
     */
    Contact(String contactID, String firstName, String lastName, String phone, String address) {
        if (contactID.length() > 10 || contactID == null) {
            throw new IllegalArgumentException("ID invalid");
        }
        else if (firstName.length() > 10 || firstName == null) {
            throw new IllegalArgumentException("Name invalid");
        }
        else if(lastName.length() > 10 || lastName == null) {
            throw new IllegalArgumentException("Description invalid");
        }
        else if (phone.length() != 10 || phone == null) {
            throw new IllegalArgumentException("Name invalid");
        }
        else if(address.length() > 30 || address == null) {
            throw new IllegalArgumentException("Description invalid");
        }
        else {
            this.contactID = contactID;
            this.firstName = firstName;
            this.lastName = lastName;
            this.phone = phone;
            this.address = address;
        }
    }
}
#include <stdlib.h>
#include <io.h> // _access
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

// Example cmd line program invocation: Inventory.exe -w 180.75 -m 4gp 42sp 69cp greatsword.json explorers-pack.json small-knife.json 2 waterskin.json leather-armor.json -c camp.log

#define FILE_PATH_BUFFER_MIN 6  // minimum chars: 'single char" + ".txt" or ".log" + '\0' => minimum 6 chars 
#define FILE_PATH_BUFFER_MAX 99 
#define MAX_ITEM_AMOUNT      76 // Wiki: beginners can have 76 slots + 2 extra can be obtained in game after achieving favor with The Coin Lords faction


typedef struct Money 
{
	int gp;
	int sp;
	int cp;
} Money;

typedef struct Node Item;
struct Node
{
	char index[50];
	char file_name[100];     // json file path
	char name[50];
	Money money;
	float weight;
	char url[100];
	char equipment_category[50];
	Item* prev;
	Item* next;
};
typedef struct Node ItemList; // Used to have a more explaining typename for the variable that holds the node list

typedef struct Inventory
{
	float max_weight;
	Money money;
	uint8_t item_count;
	ItemList* items;
	char item_file_paths[MAX_ITEM_AMOUNT][50]; 
	char log_file_name[26];   // Min buffer length: 6, Max buffer length: 100 => Parser only handles up to 99 chars + '\0'!
} Inventory;

#ifdef _WIN32 // Windows system
void ClearScreen(void)
{
	system("cls");
}
#else // Linux system
void ClearScreen(void)
{
	system("clear");
}
#endif

// MAIN ARGUMENT PARSING
void PrintProgramArgs(int argc, char* argv[]);
void ParseProgramArgs(int argc, char* argv[], Inventory* inventory);
bool IsFloat(char* float_string);

// JSON FILE PARSING
Item* JsonParse(char* file_path);

// GAME ITEM LIST / INVENTORY
ItemList* ItemListCreate(Item* new_item);
Item* ItemCreate(/*char* index*/);
void ItemPush(Inventory* inventory, Item* new_item);
void ItemPop(Inventory* inventory, char* index);      // The original Item Pointer will be set to NULL !
void ItemFree(Item** item);                           // Item** because the original pointer variable is set to NULL
void ItemPrintBasicInfo(Item* item);
void ItemPrintAdvancedInfo(Item* item);
void ItemPrintList(ItemList* items);
void ItemPrintJsonPathList(Inventory* inventory);
uint8_t InventoryGetItemCount(Inventory* inventory);
bool IsInventoryEmpty(Inventory* inventory);

void UserItemAdd(Inventory* inventory, char* file_path);

// GAME LOOP
void PrintInventoryHelpMenu(void);
void PrintItemHelpMenu(void);

// CHECK MONEY AMOUNT
// SUBTRACT MONEY
int convert_to_cp(const Money* money);
Money convert_from_cp(int total_cp);
int subtract_money(const Money* available, const Money* cost, Money* remaining);
void add_money(const Money* available, const Money* cost, Money* inventory_money);

void WriteToLogFile(Inventory* inventory, char* log_file_path);


int main(int argc, char* argv[])
{
	// TO DO: SAVE THE CURRENT JSON PATH LIST TO THE CAMP FILE ON PROGRAM EXIST, THIS WAY THE USER CAN ENTER THE FILES WHERE HE LEFT THE INVENTORY THE LAST TIME

	printf("\nDND Inventory app:\n\n");

	// printf("Executable name: %s\n", argv[0]);

	PrintProgramArgs(argc, argv); 

	Inventory inventory = { 0 };
	for (int i = 0; i < MAX_ITEM_AMOUNT; ++i)
		*(inventory.item_file_paths[i]) = '\0';

	ParseProgramArgs(argc, argv, &inventory); 

	uint8_t item_amount_to_push = 0;
	while (*(inventory.item_file_paths[item_amount_to_push]) != '\0')
		++item_amount_to_push;
	printf("Amount of json files to parse: %d\n\n", item_amount_to_push);

	ItemPrintJsonPathList(&inventory);

	printf("Parsing Json files to Item objects.\n");

	Item* new_item = NULL;
	for (int i = 0; i < item_amount_to_push; ++i)
	{
		new_item = JsonParse(inventory.item_file_paths[i]);

		printf("\nNew Item created from JSON file. Index: %s, Name: %s\n\n", new_item->index, new_item->name); 
		ItemPush(&inventory, new_item); 
		printf("\n");
	}

	bool exit_inventory = false;
	bool view_item_one_by_one = false;
	char user_input = '\0';

	printf("Inventory app start:\n");
	PrintInventoryHelpMenu();

	while (!exit_inventory)
	{
		scanf(" %c", &user_input); // NOTE: THE LEADING SPACE BEFORE THE CHARACTER SPECIFIER IN THE FORMAT STRING REMOVES ISSUES WITH CHARACTERS LIKE TRAILING NEW LINES IN THE USER INPUT.

		switch (user_input)
		{
		case 'a':
		case 'A':
			printf("Total item amount: %d\n", inventory.item_count);
			break;
		case 'c':
		case 'C':
			ClearScreen();
			break;
		case 'h':
		case 'H':
			PrintInventoryHelpMenu();
			break;
		case 'i':
		case 'I':
			view_item_one_by_one = true;
			PrintItemHelpMenu();

			Item* current_item = inventory.items; // SET THE FIRST ITEM IN THE LIST AS ITEM TO VIEW
			ItemPrintBasicInfo(current_item); // PRINT BASIC INFO ABOUT THE FIRST ITEM.

			while (view_item_one_by_one) 
			{
				scanf(" %c", &user_input); // NOTE: THE LEADING SPACE BEFORE THE CHARACTER SPECIFIER IN THE FORMAT STRING REMOVES ISSUES WITH CHARACTERS LIKE TRAILING NEW LINES IN THE USER INPUT.

				switch (user_input)
				{
					case 'c':
					case 'C':
						ClearScreen();
						break;
					case 'd':
					case 'D':
						ItemPrintAdvancedInfo(current_item);
						break;
					case 'h':
					case 'H':
						PrintItemHelpMenu(); 
						break;
					break;
					case 'n':
					case 'N':
						if (current_item)
						{
							current_item = current_item->next;
							ItemPrintBasicInfo(current_item);
						}
						else
						{
							printf("No items available to display.\n");
						}
						break;
					case 'p':
					case 'P':
						if (current_item)
						{
							current_item = current_item->prev;
							ItemPrintBasicInfo(current_item);
						}
						else
						{
							printf("No items available to display.\n");
						}
						break;
					case 'q':
					case 'Q':
						view_item_one_by_one = false;
						printf("Quiting item view.\n");
						PrintInventoryHelpMenu();
						break;
					case 'x':
					case 'X':
						if (current_item)
						{
							bool user_answered = false;
							while (!user_answered)
							{
								printf("Are you sure you want to delete this item ? ( N / Y )\n");
								scanf(" %c", &user_input); // NOTE: THE LEADING SPACE BEFORE THE CHARACTER SPECIFIER IN THE FORMAT STRING REMOVES ISSUES WITH CHARACTERS LIKE TRAILING NEW LINES IN THE USER INPUT.

								switch (user_input)
								{
								case 'n':
								case 'N':
									ItemPrintBasicInfo(current_item);
									user_answered = true;
									break;
								case 'y':
								case 'Y': // TO DO: ADJUST MONEY AND WEIGHT 
									if (current_item)
									{
										Item* temp = current_item->prev; // SAVE THE PREVIOUS ITEM IN THE LIST TO SHOW IT'S INFORMATION WHEN THE CURRENT ITEM IS DELETED
										ItemPop(&inventory, current_item->index);

										if (IsInventoryEmpty(&inventory))
											current_item = NULL;
										else
											current_item = temp;

										ItemPrintBasicInfo(current_item);
									}
									else
									{
										printf("The list is empty.\n");
									}
									user_answered = true;
									break;
								default:
									printf("Non valid command enterd, please enter yes (Y) or no (N).\n");
									break;
								}
							}
						}
						else
						{
							printf("No items available to delete.\n");
						}

						break;
					default:
						printf("Non valid command entered.\n");
						break;
				}
			}

			break;
		case 'l': // Note: Lower case letter l, not number 1
		case 'L':
			ItemPrintList(inventory.items);
			break;
		case 'm':
		case 'M':
			printf("Money amount: %dgp %dsp %dcp\n", inventory.money.gp, inventory.money.sp, inventory.money.cp);
			break;
		case 'n':
		case 'N':
			printf("Enter json file name of the item to add. Please make sure the file is located in the folder: Items_JSON. Example: sword.json\n");
			char file_name[50];
			// fgets(file_name, sizeof(file_name), stdin);
			scanf("%s", file_name);
			UserItemAdd(&inventory, file_name);
			break;
		case 'q':
		case 'Q':
			bool user_answered = false;
			while (!user_answered)
			{
				printf("Are you sure you want to quit the inventory ? ( N / Y )\n");
				scanf(" %c", &user_input); // NOTE: THE LEADING SPACE BEFORE THE CHARACTER SPECIFIER IN THE FORMAT STRING REMOVES ISSUES WITH CHARACTERS LIKE TRAILING NEW LINES IN THE USER INPUT.

				switch (user_input)
				{
				case 'n':
				case 'N':
					PrintInventoryHelpMenu();
					user_answered = true;
					break;
				case 'y':
				case 'Y': 
					exit_inventory = true;
					user_answered = true;
					break;
				default:
					printf("Non valid command enterd, please enter yes (Y) or no (N).\n");
					break;
				}
			}
			break;
		case 'w':
		case 'W':
			printf("Carring weight left: %.2f\n", inventory.max_weight);
			break;
		default:
			printf("Non valid command entered.\n");
			break;
		}
	}

	printf("Quiting inventory app.");
	return 0;
}

void PrintProgramArgs(int argc, char* argv[])
{
	printf("Program arguments:\n");
	for (int i = 1; i < argc; ++i)
		// printf("%s ", argv[i]);
		printf("%s ", *(argv + i));
	printf("\n\n");
}

void ParseProgramArgs(int argc, char* argv[], Inventory* inventory)
{
	for (int i = 1; i < argc; ++i)
	{
		// TO DO: STORE *(argv + i) IN A VARIABLE FOR READABLITY

		if (strcmp(*(argv + i), "-w") == 0) // Inventory max weight
		{	
			++i; // Proceed the loop to check if the next string is a valid number (int or float)

			if (IsFloat(*(argv + i)))
			{
				inventory->max_weight = strtof(*(argv + i), NULL); // Note: strtof() is locale dependend so watch out for '.' vs ',' decimal points!
				printf("Inventory max weight: %.2f\n", inventory->max_weight);
			}
			else
			{
				printf("Invalid max weight entered. Example: -w 25.5\nExiting program.\n");
				exit(1); // TO DO: CHOOSE EXIT NUMBERS FOR EACH POSSIBLE EXIT
			}
		}
		else if (strcmp(*(argv + i), "-m") == 0) // Inventory money: example format -m 4gp 42sp 69cp 
		{
			++i; // Proceed the loop to check if the following string has the format "%dgp"

			char money_type[3]; // "gp\0" or "sp\0" or "cp\0"
			if (sscanf(*(argv + i), "%d%2s", &(inventory->money.gp), &money_type) == 2) // Note only read 2bytes of money type to prevent buffer overflow => Check if the money type string endend after 2 chars, else the string is too long and can't be a valid money type
			{
				if(strcmp("gp", money_type) == 0 && money_type[2] == '\0')
				{
					printf("Money gp: %d\n", inventory->money.gp);
					++i; // Proceed the loop to check if the following string has the format "%dsp"
				}
				else // NON VALID INTEGER ENTERED
				{
					printf("Invalid money 'gp' format entered. Example format: -m 4gp 42sp 69cp\nExiting program.\n");
					exit(1); // TO DO: CHOOSE EXIT NUMBERS FOR EACH POSSIBLE EXIT
				}
			}
			else // NON VALID INTEGER ENTERED
			{
				printf("Invalid money 'gp' format entered. Example format: -m 4gp 42sp 69cp\nExiting program.\n");
				exit(1); // TO DO: CHOOSE EXIT NUMBERS FOR EACH POSSIBLE EXIT
			}

			if (sscanf(*(argv + i), "%d%2s", &(inventory->money.sp), &money_type) == 2)
			{
				if (strcmp("sp", money_type) == 0 && money_type[2] == '\0')
				{
					printf("Money sp: %d\n", inventory->money.sp);
					++i; // Proceed the loop to check if the following string has the format "%dcp"
				}
				else
				{
					printf("Invalid money 'sp' format entered. Example format: -m 4gp 42sp 69cp\nExiting program.\n");
					exit(1); // TO DO: CHOOSE EXIT NUMBERS FOR EACH POSSIBLE EXIT
				}
			}
			else
			{
				printf("Invalid money 'sp' format entered. Example format: -m 4gp 42sp 69cp\nExiting program.\n");
				exit(1); // TO DO: CHOOSE EXIT NUMBERS FOR EACH POSSIBLE EXIT
			}

			if (sscanf(*(argv + i), "%d%2s", &(inventory->money.cp), &money_type) == 2)
			{
				if (strcmp("cp", money_type) == 0 && money_type[2] == '\0')
				{
					printf("Money cp: %d\n", inventory->money.cp);
				}
				else
				{
					printf("Invalid money 'cp' format entered. Example format: -m 4gp 42sp 69cp\nExiting program.\n");
					exit(1); // TO DO: CHOOSE EXIT NUMBERS FOR EACH POSSIBLE EXIT
				}
			}
			else
			{
				printf("Invalid money 'cp' format entered. Example format: -m 4gp 42sp 69cp\nExiting program.\n");
				exit(1); // TO DO: CHOOSE EXIT NUMBERS FOR EACH POSSIBLE EXIT
			}
		}
		else if (strcmp(*(argv + i), "-c") == 0) // Inventory camp log file
		{
			// 1.) ALLOW THE PROGRAMMER TO ADJUST THE LOG FILENAME LENGTH IN THE INVENTORY STRUCT FROM 6chars TO 99chars:
			int filename_max_size = sizeof(inventory->log_file_name) - 1; // -1 for the '\0' char
			if(filename_max_size < FILE_PATH_BUFFER_MIN || filename_max_size > FILE_PATH_BUFFER_MAX) // minimum chars: 'single char" + ".txt" or ".log" + '\0' => minimum 6 chars 
			{
				printf("Invalid Log file name buffer length => min characters: %d, max characters: %d\nExiting program!", FILE_PATH_BUFFER_MIN, FILE_PATH_BUFFER_MAX);
				exit(1); // TO DO: CHOOSE EXIT NUMBERS FOR EACH POSSIBLE EXIT
			}

			int snprintf_ret = -1;

			// 2.) CHECK IF THE CAMP LOG FILENAME ENTERED BY THE USER IS VALID:
			++i; // Proceed the loop to check if the following string is a valid .txt or .log filename

			snprintf_ret = snprintf(inventory->log_file_name, sizeof(inventory->log_file_name), "%s", *(argv + i)); // Try to copy the entered log file path name to the Inventory struct and retreive the length (excl '\0') in the snprintf() return
			if(snprintf_ret > 0 && snprintf_ret < sizeof(inventory->log_file_name)) // Check if the copy succeed and if the length of the entered file path name isnt't to large
			{
				if (snprintf_ret >= FILE_PATH_BUFFER_MIN - 1) // minimum 5 chars needed for a valid path: 'single char" + ".txt" or ".log"
				{
					int filename_str_length = snprintf_ret;

					// printf("Camp log file name: %s\nCamp log file name length: %d\n", inventory->log_file_name, filename_str_length);
					
					if (!strcmp(inventory->log_file_name + filename_str_length - 4, ".log")) // Check if the file path name ends with ".log" or ".txt"
						;// printf("File name extension: .log\n");
					else if (!strcmp(inventory->log_file_name + filename_str_length - 4, ".txt"))
						;// printf("File name extension: .txt\n");
					else
					{
						printf("Log file name doesn't have a valid extension! => please use .txt or .log!\nExiting program.\n");
						exit(1); // TO DO: CHOOSE EXIT NUMBERS FOR EACH POSSIBLE EXIT
					}
					
					char c = '\0';
					for (int index = 0; index < filename_str_length - 4; ++index) // Loop over the log file path (skip extension .log or .txt) to check for invalid characters. Valid characters: Letters, integers and underscore
					{
						c = *((*(argv + i)) + index);
						// printf("Character: %c\n", c);
						if (!(isalpha(c) || c == '_'))
						{
							printf("Invalid character used in file name! => Valid characters: Letters, integers and underscore ('_')\nExiting program.\n");
							exit(1); // TO DO: CHOOSE EXIT NUMBERS FOR EACH POSSIBLE EXIT
						}
					}
				}
				else
				{
					printf("Log file name to short ! => min 1 char + \".log\" or \".txt\"\nExiting program.\n");
					exit(1); // TO DO: CHOOSE EXIT NUMBERS FOR EACH POSSIBLE EXIT
				}
			}
			else
			{
				printf("Log file name to long ! => max characters: %d\nExiting program.\n", filename_max_size);
				exit(1); // TO DO: CHOOSE EXIT NUMBERS FOR EACH POSSIBLE EXIT
			}
		}
		else // Inventory items or unknown commands
		{
			size_t json_filename_len = strlen(*(argv + i));
			if (json_filename_len < 6) // A json file name needs at least 6 chars to be valid: minimun 1char + 5chars for ".json"
			{
				printf("Entered unknown command: %s => this command is ignored!\n", *(argv + i));
				// exit(1); // Just ignore the command and keep the program running as long as the other commands are valid
				continue;
			}

			if (!strcmp(*(argv + i) + json_filename_len - 5, ".json")) // Check if the string contains ".json"
			{
				// COMBINE FOLDER NAME WITH FILENAME: "Items_JSON\\FILENAME"
				char json_item_folder_name[11] = "Items_JSON";
				unsigned int json_item_full_path_len = (sizeof (json_item_folder_name) + json_filename_len + 1); // sizeof includes '\0' char, +1 for the backslash '\'
				char json_item_full_path[json_item_full_path_len]; 

				int snprintf_ret = snprintf(json_item_full_path, json_item_full_path_len, "%s\\%s", json_item_folder_name, *(argv + i));
				
				if (snprintf_ret < 0 && snprintf_ret >= json_item_full_path_len)
				{
					printf("Failed to copy full json item path name!\nExiting program!\n");
					exit(1);
				}
				// printf("Full json path: %s, length; %d\n", json_item_full_path, json_item_full_path_len);

				// Check for existence
				if ((_access(json_item_full_path, 0)) != -1)
				{
					static int total_item_amount = 0;

					printf("File %s exists\n", json_item_full_path);
					// Check for read permission
					if ((_access(json_item_full_path, 4)) != -1)
					{
						// printf("File %s has read permission\n", json_item_full_path);

						++i; // Proceed the loop to check if the following string is an integer. If it is, this is the item amount
						
						int item_amount = 1;
						if (sscanf(*(argv + i), "%d", &item_amount) != 1) // If the next argv string isn't an integer, the item is just included once
						{
							--i; // Decrease the loop index again so the main for loop can do another check on this argv string to check if it is a valid command
							item_amount = 1;
						}
						
						// printf("Iventory current item amount: %d\n", inventory->item_count);
						for (int i = total_item_amount; i < total_item_amount + item_amount; ++i) // Copy the full path of the json file to the json file path list of the inventory to parse these files after all the arguments are parsed.
						{
							// printf("Loop index: %d, Trying to copy full json path to inventory file path array.\n", i);
							snprintf_ret = snprintf(inventory->item_file_paths[i], json_item_full_path_len, "%s", json_item_full_path);
							if (snprintf_ret < 0 && snprintf_ret >= json_item_full_path_len)
								printf("Failed to copy full json item path name!\nExiting program!\n");
							// else
								// printf("Copying full json path succeeded: %s\n", inventory->item_file_paths[i]);
						}
						total_item_amount += item_amount;

						printf("Item amount to add: %d\n", item_amount);
					}
				}
				else
				{
					// printf("%s does not exist!\nExiting program\n", *(argv + i));
					// exit(1);
					printf("%s does not exist! Item is ignored!\n", *(argv + i));
				}
			}
			else
			{
				printf("Entered unknown command: %s => this command is ignored!\n", *(argv + i));
				// exit(1); // Just ignore the command and keep the program running as long as the other commands are valid
			}
		}
	}

	printf("\n");
}

bool IsFloat(char* float_string) // Only accepts '.' as decimal point
{
	bool has_decimal_point = false;
	char c = '\0';

	uint8_t i = 0;
	while ((c = *(float_string + i)) != '\0')
	{
		if (!isdigit(c) && c != '.')
		{
			return false;
		}
		else if (c == '.') // Avoid double decimal points => example: 15.25.13
		{
			if (!has_decimal_point)
				has_decimal_point = true;
			else
				return false;
		}

		++i;
	}

	return true;
}

Item* JsonParse(char* file_path)
{
	if (file_path)
	{
		Item* item = ItemCreate();
		printf("Parsing file: %s\n", file_path);
		FILE* file = fopen(file_path, "r");

		if (file)
		{
			printf("Scanning file for the index.\n");

			// READ THE WHOLE FILE INTO AN ARRAY
			char file_buffer[1000] = { 0 };
			char* buffer_pointer = file_buffer; // POINTER TO TRAVERSE THE JSON STRING WITH

			fseek(file, 0, SEEK_END);           // SEARCH FOR THE END OF THE FILE
			long file_length = ftell(file);	    // STORE THE AMOUNT OF CHARACTERS IN THE FILE
			fseek(file, 0, SEEK_SET);           // REPOINT THE FILE POINTER TO THE FIRST CHARACTER OF THE FILE
			if (( sizeof file_buffer - 1) < file_length)
			{
				printf("Error: Buffer isn't large enough to store the json file content.\n");
				exit(4);
			}
			else
			{
				fread(file_buffer, 1, file_length, file);
				file_buffer[file_length] = '\0';
			}

			// printf("%s\n", file_buffer); // PRINT THE FULL JSON STRING

			// TO DO: IF NEW OBJECT KEY IS FOUND, RESET THE JSON_KEY_INDEX TO 0
			// uint8_t json_obj_index_count = 0; // !!!!! ONLY THE MAIN INDEX FOR NOW !!!!!! KEEP TRACK OF THE INDEX COUNT OF AN OBJECT, SOME OBJECTS CONSIST OF MUTIPLE OBJECTS SO THEY HAVE MORE THAN 1 INDEX KEYS
			bool is_main_index_found = false; // USED TO SEE IF THE CURRENT MEMBERS ARE FROM THE MAIN JSON OBJECT OR FROM A NESTED OBJECT => TO DO: DETECT THE END OF A NESTED OBJECT
			// bool is_new_object_found = false; // SET A FLAG WHEN A NEW JSON OBJECT STARTS
			bool is_main_name_found = false;
			bool is_main_weight_found = false;
			bool is_cost_found = false;
			bool is_cost_quantity_found = false;
			bool is_cost_unit_found = false;
			int coin_amount = -1;
			char money_unit[3] = { 0 };
			bool is_equipment_category_found = false;
			bool is_equipment_category_index_found = false;
			bool is_main_url_found = false;

			bool is_open_accolade = false;
			bool is_closing_accolade = false;
			bool is_opening_square_bracket = false;
			bool is_open_quote = false;
			bool is_closing_quote = false;
			bool is_colon_reached = false; 
			bool is_key = false;
			bool is_digit = false;

			char key[50] = { 0 };
			char value[50] = { 0 };
			int index = 0;
			while (*buffer_pointer != '\0')
			{
				if (*buffer_pointer == ' ' && !is_open_quote)	  // SKIP WHITESPACE => WHY DOES IT CRASH WITHOUT THIS ?????
				{
					++buffer_pointer;
					continue;
				}
				if (*buffer_pointer == '"' && !is_open_quote)     // OPENINGS "
				{
					is_open_quote = true;
					++buffer_pointer;
					continue;
				}
				else if (*buffer_pointer == '"' && is_open_quote) // CLOSINGS "
				{
					is_open_quote = false;

					if (is_key)
					{
						key[index] = '\0';
						if (strcmp(key, "index") == 0 && !is_main_index_found) // printf("Main index found\n");
							is_main_index_found = true;
						else if (strcmp(key, "name") == 0 && !is_main_name_found)
							is_main_name_found = true;
						else if (strcmp(key, "weight") == 0 && !is_main_weight_found)
							is_main_weight_found = true;
						else if (strcmp(key, "quantity") == 0 && !is_cost_quantity_found)
							is_cost_quantity_found = true;
						else if (strcmp(key, "unit") == 0 && is_cost_found && is_cost_quantity_found && !is_cost_unit_found)
							is_cost_unit_found = true;
						else if (strcmp(key, "index") == 0 && is_equipment_category_found)
							is_equipment_category_index_found = true;
						else if (strcmp(key, "url") == 0  && !is_main_url_found)
							is_main_url_found = true;
					}
					else
					{
						value[index] = '\0';
						// TO DO: CHECK IF THE KEY FLAG FOR INDEX IS FOUND INDEX, IF SO, ADD IT TO THE ITEM
						if (item->index[0] == '\0' && is_main_index_found) 
						{
							strcpy(item->index, value);
							// printf("Added the main index to the item: %s.\n", item->index);
						}
						else if (item->name[0] == '\0' && is_main_name_found) 
						{
							strcpy(item->name, value);
							// printf("Added the main name to the item: %s.\n", item->name);
						}
						else if (money_unit[0] == '\0' && is_cost_found && is_cost_unit_found)
						{
							strcpy(money_unit, value);
							printf("Money unit is found: %s.\n", money_unit);
							if (strcmp(money_unit, "gp") == 0)
								item->money.gp = coin_amount;
							else if (strcmp(money_unit, "sp") == 0)
								item->money.sp = coin_amount;
							else if (strcmp(money_unit, "cp") == 0)
								item->money.cp = coin_amount;
							// printf("Item money should be: %dgp, %dsp, %dcp.\n", item->money.gp, item->money.sp, item->money.cp);
						}
						else if (is_equipment_category_found) // THE MAIN URL IS ALWAYS THE LAST KEY VALUE PAIR WITH THE KEY URL. THIS STORE AL URLS IT WILL COME BY BUT EVENTUALLY WILL HOLD THE LAST URL.
						{
							strcpy(item->equipment_category, value);
							is_equipment_category_found = false;
						}
						else if (is_main_url_found) // THE MAIN URL IS ALWAYS THE LAST KEY VALUE PAIR WITH THE KEY URL. THIS STORE AL URLS IT WILL COME BY BUT EVENTUALLY WILL HOLD THE LAST URL.
						{
							strcpy(item->url, value);
							is_main_url_found = false;
						}

						printf("Key: %s, Value: %s\n", key, value);
					}

					index = 0;
					++buffer_pointer;
					continue;
				}
				else if (*buffer_pointer == ':')				  // KEY VALUE DELIMITER :
				{
					is_colon_reached = true;
					// index = 0;
					++buffer_pointer;
					continue;
				}
				else if (*buffer_pointer == ',')				  // KEY VALUE PAIR DELIMITER ,
				{
					if (is_digit)
					{
						is_digit = false;
						value[index] = '\0';
						if (item->weight < 0.0f && is_main_weight_found)
						{
							item->weight = (float)atof(value);
							// printf("The weight is found: %s\n", value);
							// printf("Added the main weight to the item: %.2f.\n", item->weight);
						}
						else if (coin_amount == -1 && is_cost_found && is_cost_quantity_found)
						{
							coin_amount = atoi(value);
							printf("Coin amount is found: %d.\n", coin_amount);
						}

						index = 0;
						printf("Key: %s, Value: %s\n", key, value);
					}
					is_colon_reached = false;
					++buffer_pointer;
					continue;
				}
				else if (*buffer_pointer == '{')				  // OBJECT OPENING {
				{
					if (is_colon_reached) // WHEN A COLON IS FOLLOWED BY {, A NEW OBJECT STARTS
					{
						if (strcmp(key, "cost") == 0 && !is_cost_found)
						{
							printf("Cost is found\n");
							is_cost_found = true;
						}
						else if (strcmp(key, "equipment_category") == 0 && !is_equipment_category_found) // HERE IT IS !!!!!!!!!!!
						{
							printf("equipment_category is found\n");
							is_equipment_category_found = true;
						}

						printf("Object Key: %s\n", key);
					}

					is_colon_reached = false;
					++buffer_pointer;
					continue;
				}
				else if (*buffer_pointer == '[')				  // ARRAY OPENING [
				{
					++buffer_pointer;
					continue;
				}
				else if (*buffer_pointer == ']')				  // ARRAY CLOSING ]
				{
					// TO DO: CHECK EVERYTHING IN BETWEEN THE []
					value[index] = '\0';
					printf("Key: %s, Value: %s\n", key, value);
					++buffer_pointer;
					continue;
				}
				
				if (isdigit(*buffer_pointer))
				{
					is_digit = true;
					value[index++] = *buffer_pointer;
				}
				else if (is_open_quote && !is_colon_reached)	   // SAVE THE KEY
				{
					is_key = true;
					key[index++] = *buffer_pointer;
				}
				else if (is_open_quote && is_colon_reached)       // SAVE THE VALUE
				{
					is_key = false;
					value[index++] = *buffer_pointer;
				}

				++buffer_pointer;
			}
			// printf("End of json string reached. %d chars read.\n", char_count);

			fclose(file);
		}
		else
		{
			printf("Failed to open file.\n");
			exit(3);
		}

		return item;
	}
	else
	{
		printf("Failed to parse json file. File path is NULL.\n");
		exit(3);
	}
}

ItemList* ItemListCreate(Item* new_item) 
{
	ItemList* items = new_item; // The head of the list
	items->next = items;        // Point the head to itself
	items->prev = items;

	return items;
}

Item* ItemCreate(/*char* index*/)
{
	Item* item = (Item*)calloc(1, sizeof(Item));
	item->weight = -1.0f;

	if (item)
	{
		// item->index = index;
		return item;
	}
	else
	{
		printf("Failed to allocate memory for a new Item!\nExiting program!\n");
		exit(2);
	}
}

void ItemPush(Inventory* inventory, Item* new_item) // Push item at the end of the list
{
	printf("Pushing item: %s\n", new_item->index);

	if (inventory == NULL)
	{
		printf("inventory is NULL pointer!\n");
		return;
	}

	// CHECK IF THE INVENTORY HAS ENOUGH MONEY AND WEIGHT BEFORE ADDING => ELSE GIVE A WARNING AND DONT ADD THE ITEM
	// WEIGHT CHECK
	if ((inventory->max_weight - new_item->weight) < 0.0f)
	{
		printf("Item index %s can't be added to the inventory because it exceeds the carrying capacity left. Item is not included!\n", new_item->index);
		return;
	}
	else
	{
		inventory->max_weight -= new_item->weight;
		printf("Inventory carrying capacity left: %.2f\n", inventory->max_weight);
	}
	// MONEY CHECK
	Money remaining;
	if (subtract_money(&(inventory->money), &(new_item->money), &remaining)) 
	{
		printf("Sufficient funds.\n");
		printf("Remaining: %d gp, %d sp, %d cp\n", remaining.gp, remaining.sp, remaining.cp);
		inventory->money = remaining;
	}
	else 
	{
		printf("Not enough money left to include %s. Item is not included!\n", new_item->index); 
		return;
	}



	if (inventory->items)
	{
		Item* head = inventory->items;
		Item* temp = head;
		Item* temp_prev = temp->prev;

		while (temp->next != head) // Get the current last item of the list
			temp = temp->next;

		new_item->prev = temp;     // Point the previous pointer of the newly pushed item to the previously last item.
		new_item->next = head;     // Point the next pointer of the newly pushed item to the head.
		temp->next = new_item;     // Point the previously last item to the newly pushed item.
		head->prev = new_item;     // Point the previous pointer of the head to the newly pushed (last) item.
	}
	else // If the list is a NULL pointer, create a new list
	{
		inventory->items = ItemListCreate(new_item);
	}

	++(inventory->item_count);
}

void ItemPop(Inventory* inventory, char* index) // Pop the chosen item from the list
{
	if (inventory == NULL)
	{
		printf("inventory is NULL pointer!\n");
		return;
	}

	uint8_t item_count = InventoryGetItemCount(inventory);
	if(item_count == 0)
	{
		printf("items list is empty! (NULL pointer)\n");
		return;
	}

	Item** head = &(inventory->items);
	Item* temp = *head;

	do // CHECK IF THE LIST CONTAINS THE GIVEN INDEX TO POP AND POP IT WHEN FOUND
	{
		if (strcmp(temp->index, index) == 0)     // THE ITEM INDEX IS FOUND IN THE LIST
		{
			printf("Popping item: %s\n", index);

			if (inventory->item_count == 1)      // LIST CONTAINS ONLY 1 ITEM WHICH IS THE HEAD: AFTER POPPING LIST IS EMPTY
			{
				// INCREASE THE CARRYING CAPACITY WITH THE ITEM WEIGHT
				inventory->max_weight += (*head)->weight;
				printf("Inventory carrying capacity left: %.2f\n", inventory->max_weight);
				// INCREASE IVENTORY MONEY
				add_money(&(inventory->money), &((*head)->money), &(inventory->money));

				ItemFree(head);
			}
			else if (inventory->item_count == 2) // LIST CONTAINS 2 ITEMS: AFTER POPPING THE LIST CONTAINS ONLY 1 ITEM WHICH BECOMES THE HEAD
			{
				// printf("Pop(): List has only %d item left after popping this item\n\r", inventory->item_count - 1);

				*head = temp->next;     // ASSIGN THE ONE ITEM OF THE TWO THAT ISN'T POPPED TO THE HEAD OF THE LIST. THE NOT POPPED ITEM IS ALWAYS THE NEXT OR PREV POINTER OF THE POPPED ITEM BECAUSE THERE ARE ONLY 2 ITEMS IN THE LIST AT THIS POINT
				(*head)->next = *head;  // POINT THE HEAD TO ITSELF
				(*head)->prev = *head;  // POINT THE HEAD TO ITSELF

				// INCREASE THE CARRYING CAPACITY WITH THE ITEM WEIGHT
				inventory->max_weight += temp->weight;
				printf("Inventory carrying capacity left: %.2f\n", inventory->max_weight);
				// INCREASE IVENTORY MONEY
				add_money(&(inventory->money), &(temp->money), &(inventory->money));

				ItemFree(&temp);        // FREE THE ITEM THAT NEEDS TO BE POPPED. WHEN THE HEAD NEEDS TO BE REMOVED, temp WILL HOLD THE HEAD ADDRESS. WHEN THE TAIL NEEDS TO BE REMOVED, temp WILL HOLD THE TAIL ADDRESS.
			}
			else
			{
				if (*head == temp) // IF THE 'TO BE POPPED ITEM' IS THE CURRENT HEAD, REPLACE ORIGINAL HEAD. NOTE: temp WILL HOLD THE POINTER TO THE OLD HEAD THAT IS TO BE POPPED.
				{
					(*head)->prev->next = (*head)->next;  // POINT THE LAST ITEM'S NEXT POINTER TO THE SECOND ITEM OF THE LIST.
					(*head)->next->prev = (*head)->prev;  // POINT THE SECOND ITEM'S PREV POINTER TO THE LAST ITEM OF THE LIST.
					*head = temp->next;                   // ASSIGN THE SECOND ITEM OF THE LIST TO THE OLD HEAD OF THE LIST.

					// INCREASE THE CARRYING CAPACITY WITH THE ITEM WEIGHT
					inventory->max_weight += temp->weight;
					printf("Inventory carrying capacity left: %.2f\n", inventory->max_weight);
					// INCREASE IVENTORY MONEY
					add_money(&(inventory->money), &(temp->money), &(inventory->money));

					ItemFree(&temp);                      // POP/FREE THE OLD HEAD OF THE LIST.
				}
				else
				{
					temp->prev->next = temp->next;
					temp->next->prev = temp->prev;

					// INCREASE THE CARRYING CAPACITY WITH THE ITEM WEIGHT
					inventory->max_weight += temp->weight;
					printf("Inventory carrying capacity left: %.2f\n", inventory->max_weight);
					// INCREASE IVENTORY MONEY
					add_money(&(inventory->money), &(temp->money), &(inventory->money));

					ItemFree(&temp);                      // POP/FREE THE ITEM.
				}
				
			}

			--(inventory->item_count);                    // DECREASE THE ITEM COUNT WHEN A 'TO POPPED' INDEX IS FOUND

			return;
		}

		temp = temp->next;								  // SET THE temp POINTER TO THE NEXT POINTER OF THE CURRENT ITEM

	} while (temp != *head);

	// NOTIFY THE PLAYER IF THE INDEX IS NOT FOUND
	printf("Index: %s is not found in the list!\n\r", index);
}

void ItemFree(Item** item)
{
	if (*item)
	{
		free(*item);
		*item = NULL;
	}
}

void ItemPrintBasicInfo(Item* item)
{
	if (item)
	{
		printf("Index: %s\nName: %s\nweight: %.2f\nMoney: %dgp, %dsp, %dcp.\n", item->index, item->name, item->weight, item->money.gp, item->money.sp, item->money.cp);
	}
	else
		printf("List is empty.\n");
}

void ItemPrintAdvancedInfo(Item* item)
{
	printf("Item url: %s\nEquipment category: %s\n", item->url, item->equipment_category);
}

void ItemPrintList(ItemList* items)
{
	if (items)
	{
		Item* head = items;
		Item* temp = head;

		uint8_t item_num = 1;
		do
		{
			printf("Index: %s\nName: %s\nweight: %.2f\nMoney: %dgp, %dsp, %dcp.\n\n", temp->index, temp->name, temp->weight, temp->money.gp, temp->money.sp, temp->money.cp);
			temp = temp->next;
			++item_num;
		} while (temp != head);
	}
	else
	{
		printf("List is empty.\n");
	}
}

void ItemPrintJsonPathList(Inventory* inventory)
{
	printf("Printing json file paths:\n");
	uint8_t path_amount = 0;
	while (*(inventory->item_file_paths[path_amount]) != '\0')
	{
		printf("%s\n", inventory->item_file_paths[path_amount]);
		++path_amount;
	}
	printf("\n");
}

uint8_t InventoryGetItemCount(Inventory* inventory)
{
	if (inventory)
		return inventory->item_count;
	else
		printf("Inventory is NULL pointer!\n");
	return 0;
}

bool IsInventoryEmpty(Inventory* inventory)
{
	return (inventory->item_count == 0) ? true : false;
}

void UserItemAdd(Inventory* inventory, char* file_path)
{
	// COMBINE FOLDER NAME WITH FILENAME: "Items_JSON\\FILENAME"
	size_t json_filename_len = strlen(file_path);
	char json_item_folder_name[11] = "Items_JSON";
	unsigned int json_item_full_path_len = (sizeof(json_item_folder_name) + json_filename_len + 1); // sizeof includes '\0' char, +1 for the backslash '\'
	char json_item_full_path[json_item_full_path_len];

	int snprintf_ret = snprintf(json_item_full_path, json_item_full_path_len, "%s\\%s", json_item_folder_name, file_path);

	if (snprintf_ret < 0 && snprintf_ret >= json_item_full_path_len)
	{
		printf("Failed to copy full json item path name!\nExiting program!\n");
		exit(1);
	}

	Item* new_item = NULL;
	new_item = JsonParse(json_item_full_path);

	printf("\nNew Item created from JSON file. Index: %s, Name: %s\n\n", new_item->index, new_item->name);
	ItemPush(inventory, new_item); 
	printf("\n");
}

void PrintInventoryHelpMenu(void)
{
	printf("Inventory help menu:\n");
	printf("- Press H to display the inventory help menu.\n");
	printf("- Press M to display the current money amount.\n- Press W to display the carrying weight capacity left.\n- Press A to display item amount.\n");
	printf("- Press L to display basic item information of the whole list.\n- Press I to display item one by one.\n- Press N to add a new item.\n");
	printf("- Press C to clear the screen.\n- Press Q to quit the inventory.\n\n");
}

void PrintItemHelpMenu(void)
{
	printf("Item help menu:\n- Press H to display the item help menu.\n");
	printf("- Press D to display more data about the current item.\n- Press N to visit the next item.\n- Press P to visit the previous item.\n- Press X to delete the current item.\n- Press Q to quit the item view.\n");
	printf("- Press C to clear the screen.\n\n");
}

int convert_to_cp(const Money* money) 
{
	return (money->gp * 10000) + (money->sp * 100) + money->cp;
}

Money convert_from_cp(int total_cp) 
{
	Money result;
	result.gp = total_cp / 10000;
	total_cp %= 10000;
	result.sp = total_cp / 100;
	result.cp = total_cp % 100;
	return result;
}

int subtract_money(const Money* available, const Money* cost, Money* remaining) 
{
	int total_cp_available = convert_to_cp(available);
	// printf("Total money available in cp: %d\n", total_cp_available);
	int total_cp_cost = convert_to_cp(cost);
	// printf("Item money in cp: %d\n", total_cp_cost);

	if (total_cp_available < total_cp_cost) 
	{
		return 0; 
	}

	int total_cp_remaining = total_cp_available - total_cp_cost;
	*remaining = convert_from_cp(total_cp_remaining);
	return 1; 
}

void add_money(const Money* available, const Money* cost, Money* inventory_money)
{
	printf("Adding money back\n");

	int total_cp_available = convert_to_cp(available);
	// printf("Total money available in cp: %d\n", total_cp_available);
	int total_cp_cost = convert_to_cp(cost);
	// printf("Item money in cp: %d\n", total_cp_cost);

	int total_cp = total_cp_available + total_cp_cost;
	*inventory_money = convert_from_cp(total_cp);
}

void WriteToLogFile(Inventory* inventory, char* log_file_path)
{
	/*FILE* file = fopen(log_file_path, "a");

	if (file)
	{
		char* weigth_string = "Inventory weigth capacity left: ";
		fwrite(ltoa(weigth_string), 1, sizeof weigth_string, file);
		fwrite(inventory->max_weight, 1, sizeof inventory->max_weight, file);

		fclose(file);
	}
	else
	{
		printf("%s does not exitst.\n", log_file_path);
	}*/

}
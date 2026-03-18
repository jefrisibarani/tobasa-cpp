#pragma once

#include <vector>
#include <string>
#include <tobasa/logger.h>
#include <tobasa/util.h>
#include <tobasa/util_string.h>

namespace tbs {
namespace sql {

/** \addtogroup SQL
 * @{
 */

//! Forward declaration
template <typename SqlDriverType>
class SqlResult;

/**
 * \brief SQL Column mapping class.
 * \details ColumnMap designed to easily maps key and value of table's column from SqlResult object \n
 * SqlResult must has exatcly 2 columns
 * values in 1st and 2nd columns must unique.
 * 
 * Example:\n
 * We have object_brands table with following structure.
 * \code
 *   CREATE TABLE `object_brands`
 *   (
 *      `object_brand_id` INTEGER PRIMARY KEY AUTOINCREMENT,
 *      `object_brand` TEXT
 *   )
 * \endcode
 * 
 * Then, SELECT * FROM object_brand ORDER BT object_brand_id;  , gives us following tuples
 * \verbatim
 * object_brand_id | object_brand
 *  ----------------+--------------
 *    1             | Acer
 *    2             | HP
 *    3             | Compaq
 *    4             | Dell
 *    5             | Toshiba
 *    6             | Sony
 * 
 *   ---------------+--------------
 *  KEY            |   VALUE
 * \endverbatim
 * 
 * Creating ColumnMap object based on  SELECT * FROM object_brand ORDER BT object_brand_id
 * will help us easily mapping object_brand_id and object_brand.
 * This is useful when working with wxComboBox to display all object_brand.
 * Because wxComboBox indexed its items starts from 0, and object_brand_id not always have a n+1 increment,
 * ColumnMap will  Tranlate the correct index of object_brand or object_brand_id displayed on wxComboBox
 * to its real value in sql table.
 * 
 * Example:
 * \code
 * SqlResult *set = connection()->executeSet("SELECT * FROM object_brands ORDER BY object_brand_id");
 * OR ...
 * SqlResult *set = connection()->executeSet("SELECT object_brand_id, object_brand FROM object_brands ORDER BY object_brand_id");
 * 
 * wxComboBox *cbBrand(parentWindow);
 * ColumnMap cbBrandMap(set);
 * 
 * // set values to combobox
 * cbBrand->Clear();
 * // cbBrandMap.GetValues() will return std::vector<std::string> containing all object_brand
 * cbBrand->Append(cbBrandMap.GetValues());
 * cbBrand->SetSelection(0);
 * ...
 * value = _tableObjects->getValue("brand_id");
 * // cbBrandMap.GetKeyIndex(value) will translate object_brand_id to combobox's zero-based index
 * int brandIdx = cbBrandMap.GetKeyIndex(value);
 * cbBrand->SetSelection(brandIdx);
 * 
 * // Getvalue
 * // translate current combobox value to actual value in table
 * std::string brandID = cbBrandMap.GetKey( cbBrand->GetValue() );
 * _tableObjects->SetValue( _tableObjects->getColumnNumber("brand_id"),brandID );
 * \endcode
 * 
 * Yet,another example, with radio box
 * \code
 * -------------------------------------------------------
 * // File : wzdpagetriptype.h //
 * -------------------------------------------------------
 * 
 * #include "wx/wizard.h"
 * 
 * class WzdPageTripType;
 * 
 * #define id_WzdSelectTripType 10002
 * #define id_RadTripType 10005
 * #define id_LbltripInfo 10047
 * #define id_TxtTripSummary 10006
 * 
 * class WzdPageTripType: public wxWizardPageSimple
 * {
 *    DECLARE_DYNAMIC_CLASS( WzdPageTripType )
 *    DECLARE_EVENT_TABLE()
 * 
 * public:
 * 
 *    WzdPageTripType();
 *    WzdPageTripType( wxWizard* parent );
 *    bool Create( wxWizard* parent );
 *    ~WzdPageTripType(){}
 *    void Init();
 *    void CreateControls();
 * 
 *    ColumnMap& GetTripTypeColMap()     { return _tripTypeColMap;}
 *    ColumnMap& GetTripTypeHelpColMap() { return _tripTypeHelpColMap;}
 * 
 * private:
 * 
 *    friend class WzdNewTrip;
 *    void OnTripTypeSelected( wxCommandEvent& event );
 *    virtual bool TransferDataToWindow();
 *    virtual bool TransferDataFromWindow();
 *    wxStaticBoxSizer* _pMainSizer;
 *    wxRadioBox* _pTripType;
 *    wxStaticText* _pLblTripInfo;
 *    wxTextCtrl* _pTxtTripSummary;
 * 
 *    std::string _tripTypeId;
 *    std::string _summary;
 * 
 *    ColumnMap          _tripTypeColMap;
 *    ColumnMap          _tripTypeHelpColMap; // Help text for trip type
 * };
 * 
 * 
 * -------------------------------------------------------
 * File : wzdpagetriptype.cpp 
 * -------------------------------------------------------
 * 
 * #include <sdk/tobasapch.h>
 * 
 * #ifndef WX_PRECOMP
 *    #include <sdk/control.h>
 *    #include <sdk/sql/sqlconnection.h>
 *    #include <sdk/sql/sqlresultset.h>
 *    #include <sdk/utils/misc.h>
 * #endif //WX_PRECOMP

 * #include "tripwizarddata.h"
 * #include "wzdpagetriptype.h"
 * 
 * IMPLEMENT_DYNAMIC_CLASS( WzdPageTripType, wxWizardPageSimple )
 * BEGIN_EVENT_TABLE( WzdPageTripType, wxWizardPageSimple )
 *    EVT_RADIOBOX( id_RadTripType, WzdPageTripType::OnTripTypeSelected )
 * END_EVENT_TABLE()
 * 
 * WzdPageTripType::WzdPageTripType()
 * {
 *    Init();
 * }
 * 
 * WzdPageTripType::WzdPageTripType( wxWizard* parent )
 * {
 *    Init();
 *    Create( parent );
 * }
 * 
 * bool WzdPageTripType::Create( wxWizard* parent )
 * {
 *    SetParent(parent);
 *    wxBitmap wizardBitmap("tripwizard_choose.png");
 *    wxWizardPageSimple::Create( parent, NULL, NULL, wizardBitmap );
 *    CreateControls();
 * 
 *    if (GetSizer())
 *       GetSizer()->Fit(this);
 * 
 *    return true;
 * }
 * 
 * void WzdPageTripType::Init()
 * {
 *    _pMainSizer = NULL;
 *    _pTripType = NULL;
 *    _pLblTripInfo = NULL;
 *    _pTxtTripSummary = NULL;
 * 
 *    bool needMapping = true;
 *    SqlResult *set=0;
 * 
 *    set = _pConn->ExecuteSet("SELECT trip_type_id,\"type\" FROM scl_trip_types ORDER BY trip_type_id;");
 *    _tripTypeColMap.Init(set,needMapping);
 * 
 *    set = _pConn->ExecuteSet("SELECT trip_type_id,remark FROM scl_trip_types ORDER BY trip_type_id;");
 *    _tripTypeHelpColMap.Init(set,needMapping);
 * }
 * 
 * void WzdPageTripType::CreateControls()
 * {
 *    // fill with values from database
 *    std::vector<std::string> _pTripTypeStrings=dataSource->GetTripTypeColMap().getValues();
 * 
 *    wxStaticBox* staticBoxSizer = new wxStaticBox(this, wxID_ANY, "");
 *    _pMainSizer = new wxStaticBoxSizer(staticBoxSizer, wxVERTICAL);
 *    this->SetSizer(_pMainSizer);
 * 
 *    wxStaticText* text1 = new wxStaticText( itemWizardPageSimple1, wxID_STATIC, "Pilih Jenis Trip", wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT|wxST_NO_AUTORESIZE );
 *    _pMainSizer->Add(text1, 0, wxGROW|wxALL, 5);
 * 
 *    _pTripType = new wxRadioBox( this, id_RadTripType, "", wxDefaultPosition, wxDefaultSize, _pTripTypeStrings, 1, wxRA_SPECIFY_COLS );
 *    _pTripType->SetSelection(1);
 *    _pMainSizer->Add(_pTripType, 0, wxGROW|wxALL, 5);
 * 
 *    _pLblTripInfo = new wxStaticText( this, id_LbltripInfo, "Trip jenis ini hanya melakukan  satu kali perjalanan\ntanpa reposisi", wxDefaultPosition, wxSize(-1, 50), 0 );
 *    _pLblTripInfo->Wrap(250);
 *    _pMainSizer->Add(_pLblTripInfo, 0, wxGROW|wxALL, 5);
 * 
 *    _pMainSizer->Add(5, 5, 0, wxGROW|wxALL, 5);
 * 
 *    wxStaticText* text7 = new wxStaticText( itemWizardPageSimple1, wxID_STATIC, "Deskripsi", wxDefaultPosition, wxDefaultSize, 0 );
 *    _pMainSizer->Add(text7, 0, wxALIGN_LEFT|wxALL, 5);
 * 
 *    _pTxtTripSummary = new wxTextCtrl( this, id_TxtTripSummary, "Perjalan ke Tjiwi Kimia Surabaya melalui jalur utara", wxDefaultPosition, wxSize(-1, 50), wxTE_MULTILINE|wxBORDER_STATIC );
 *    _pMainSizer->Add(_pTxtTripSummary, 0, wxGROW|wxALL, 5);
 * 
 *    _pTxtTripSummary->Clear();
 * }
 * 
 * bool WzdPageTripType::TransferDataToWindow(){return true;}
 * 
 * bool WzdPageTripType::TransferDataFromWindow()
 * {
 *    _tripTypeId    = GetTripTypeColMap().getKey(_pTripType->GetSelection());
 *    _summary       = _pTxtTripSummary->GetValue().MakeUpper();
 *    return true;
 * }
 * 
 * void WzdPageTripType::OnTripTypeSelected( wxCommandEvent& event )
 * {
 *    _pLblTripInfo->SetLabel( GetTripTypeHelpColMap().getValue(_pTripType->GetSelection()) );
 * }
 * \endcode
 */
template < typename SqlDriverType >
class ColumnMap
{
public:
   using SqlResult = sql::SqlResult<SqlDriverType>;

   /// Construct a new Column Map object.
   ColumnMap()
      : _rows(0)
      , _cols(0)
   {
      _valid = false;
      _needMapping = true;
   }

   /// Destroy the Column Map object.
   virtual ~ColumnMap() {}

   /** 
    * Init from SqlResult and take the ownership.
    * \param dataset
    * \param needMapping
    */
   void init(const SqlResult& dataset, bool needMapping = true)
   {
      _keys.clear();
      _values.clear();

      _rows = dataset.totalRows();
      _cols = dataset.totalColumns();
      dataset.moveFirst();

      _needMapping = needMapping;

      int totalRows = 0;
      std::string key;
      std::string value;

      while (!dataset.isEof())
      {
         key = dataset.getStringValue(0);
         value = dataset.getStringValue(1);
         _keys.push_back(key);
         _values.push_back(value);

         totalRows++;
         dataset.moveNext();
      }

      _rows  = totalRows;
      _valid = true;
   }

   /// Get the Count object.
   inline int getCount() { return _rows; }

   /// Is valid.
   inline bool isValid() const { return _valid; }

   /// Need mapping.
   inline bool needMapping()const { return _needMapping; }

   /// Get values.
   inline std::vector<std::string> getValues() const { return _values; }

   /// Get Keys.
   inline std::vector<std::string> getKeys() const { return _keys; }

   /// Get value by key.
   std::string getValue(const std::string& key) const
   {
      if (key.empty())
         return "";

      int keyPosition = getKeyPosition(key);
      if (keyPosition == -1) {
         return "";
      }

      return getValue(keyPosition);
   }

   /** 
    * Get the value.
    * \param index zero based index of the key in control (ex: combobox)
    */
   std::string getValue(const int index) const
   {
      if (index > _rows)
      {
         Logger::logD("ColumnMap: getValue: {}: index not found, not in range", index);
         return "";
      }

      return _values.at(index);
   }

   /** 
    * Get Key by value.
    * \param value
    * \return std::string
    */
   std::string getKey(const std::string& value) const
   {
      if (value.empty())
         return "";

      int valPosition = getValuePosition(value);
      if (valPosition == -1) {
         return "";
      }

      return getKey(valPosition);
   }

   /** 
    * Get Key by index. 
    * \param index
    * \return key 
    */
   std::string getKey(const int index) const
   {
      if (index > _rows)
      {
         Logger::logD("ColumnMap: getKey: {}: index not found, not in range", index);
         return "";
      }

      return _keys.at(index);
   }

   /** 
    * Get Key by value, return as integer.
    * \param value
    */
   int getKeyInt(const std::string& value) const
   {
      return std::stoi(getKey(value));
   }

   /** 
    * Get key by position, return as integer
    * \param index
    */
   int getKeyInt(const int index) const
   {
      return std::stoi(getKey(index));
   }

   /** 
    * Get value position in a vector
    * \param value
    */
   int getValuePosition(const std::string& value) const
   {
      int position = tbs::util::findPositionInVector(_values, value);
      if (position == -1)
         Logger::logD("ColumnMap: getValuePosition: {}: value not found, not in array", value);

      return position;
   }

   /** 
    * Get Key position in avector
    * \param key
    */
   int getKeyPosition(const std::string& key) const
   {
      int position = tbs::util::findPositionInVector(_keys, key);
      if (position == -1)
         Logger::logD("ColumnMap: getKeyPosition: {}: key not found, not in array", key);

      return position;
   }

private:

   int _rows;
   int _cols;
   std::vector<std::string> _keys;
   std::vector<std::string> _values;
   bool _valid;
   bool _needMapping;
};

/** @}*/

} // namespace sql
} // namespace tbs